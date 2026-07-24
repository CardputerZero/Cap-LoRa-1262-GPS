#include "gps/gps_backend.hpp"

#include "hal/ext5v_power.hpp"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <thread>
#include <unistd.h>

namespace cap_gps::gps {
namespace {

constexpr const char* kDefaultDevice = "/dev/ttyS0";
constexpr int kDefaultBaud           = 115200;

std::string envOrDefault(const char* name, const char* fallback)
{
    const char* value = std::getenv(name);
    return value && value[0] != '\0' ? value : fallback;
}

std::string readTextFile(const char* path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::string basenameOf(const std::string& path)
{
    const auto separator = path.find_last_of('/');
    return separator == std::string::npos ? path : path.substr(separator + 1);
}

bool occupiedByConsole(const std::string& device)
{
    const std::string name    = basenameOf(device);
    const std::string cmdline = readTextFile("/proc/cmdline");
    if (cmdline.find("console=" + name) != std::string::npos || cmdline.find("console=serial0") != std::string::npos) {
        return true;
    }

    std::istringstream consoles(readTextFile("/proc/consoles"));
    std::string line;
    while (std::getline(consoles, line)) {
        if (line.find(name) != std::string::npos) {
            return true;
        }
    }
    return false;
}

class LinuxSerialBackend final : public GpsBackend {
public:
    ~LinuxSerialBackend() override
    {
        close();
    }

    bool open(ReceiverInfo& info, std::string& error) override
    {
        close();
        error.clear();
        _device = envOrDefault("CAP_GPS_UART_DEVICE", kDefaultDevice);

        if (occupiedByConsole(_device)) {
            error = _device + " is assigned to the serial console; disable serial login before using GPS";
            return false;
        }
        if (!_power.enable(error)) {
            error = "Cap power enable failed: " + error;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        spdlog::info("GPS backend: opening {} at {} 8N1 (read-only, nonblocking)", _device, kDefaultBaud);
        _fd = ::open(_device.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
        if (_fd < 0) {
            error = "open " + _device + ": " + std::strerror(errno);
            close();
            return false;
        }
        if (::flock(_fd, LOCK_EX | LOCK_NB) < 0) {
            error = "lock " + _device + ": " + std::strerror(errno);
            close();
            return false;
        }
        _locked = true;
        if (::ioctl(_fd, TIOCEXCL) < 0) {
            error = "request exclusive access to " + _device + ": " + std::strerror(errno);
            close();
            return false;
        }

        if (::tcgetattr(_fd, &_previous_termios) < 0) {
            error = "tcgetattr " + _device + ": " + std::strerror(errno);
            close();
            return false;
        }
        _have_previous_termios = true;

        termios configuration = _previous_termios;
        ::cfmakeraw(&configuration);
        configuration.c_cflag |= CLOCAL | CREAD;
        configuration.c_cflag &= static_cast<tcflag_t>(~(PARENB | CSTOPB | CSIZE));
        configuration.c_cflag |= CS8;
#ifdef CRTSCTS
        configuration.c_cflag &= static_cast<tcflag_t>(~CRTSCTS);
#endif
        configuration.c_cc[VMIN]  = 0;
        configuration.c_cc[VTIME] = 0;
        if (::cfsetispeed(&configuration, B115200) < 0 || ::cfsetospeed(&configuration, B115200) < 0 ||
            ::tcsetattr(_fd, TCSANOW, &configuration) < 0) {
            error = "configure " + _device + " for 115200 8N1: " + std::strerror(errno);
            close();
            return false;
        }
        (void)::tcflush(_fd, TCIFLUSH);

        info.backend = "Linux UART";
        info.device  = _device;
        info.baud    = kDefaultBaud;
        info.mock    = false;
        spdlog::info("GPS backend: ready on {} (EXT5V={}, SPI unused)", _device, _power.path());
        return true;
    }

    ReadResult readAvailable(uint8_t* data, std::size_t capacity) override
    {
        if (_fd < 0) {
            return {0, "GPS UART is not open"};
        }
        if (!data || capacity == 0) {
            return {};
        }

        const ssize_t count = ::read(_fd, data, capacity);
        if (count > 0) {
            return {static_cast<std::size_t>(count), {}};
        }
        if (count == 0 || errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
            return {};
        }
        return {0, "read " + _device + ": " + std::strerror(errno)};
    }

    void close() noexcept override
    {
        if (_fd >= 0) {
            spdlog::info("GPS backend: closing {}", _device);
            if (_have_previous_termios) {
                (void)::tcsetattr(_fd, TCSANOW, &_previous_termios);
            }
            if (_locked) {
                (void)::flock(_fd, LOCK_UN);
            }
            (void)::close(_fd);
        }
        _fd                    = -1;
        _locked                = false;
        _have_previous_termios = false;
        _power.disable();
    }

    bool isOpen() const noexcept override
    {
        return _fd >= 0;
    }

private:
    hal::Ext5vPower _power;
    std::string _device{kDefaultDevice};
    int _fd = -1;
    termios _previous_termios{};
    bool _have_previous_termios = false;
    bool _locked                = false;
};

}  // namespace

std::unique_ptr<GpsBackend> makeLinuxSerialBackend()
{
    return std::make_unique<LinuxSerialBackend>();
}

}  // namespace cap_gps::gps
