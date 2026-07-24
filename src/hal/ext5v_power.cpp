#include "hal/ext5v_power.hpp"

#include <spdlog/spdlog.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <system_error>
#include <unistd.h>

namespace cap_gps::hal {
namespace {

constexpr const char* kDefaultBrightnessPath = "/sys/class/leds/ext_5v_out/brightness";

bool envEnabled(const char* name, bool fallback)
{
    const char* value = std::getenv(name);
    if (!value || value[0] == '\0') {
        return fallback;
    }
    return std::strcmp(value, "0") != 0 && std::strcmp(value, "false") != 0 && std::strcmp(value, "off") != 0;
}

std::string envOrDefault(const char* name, const char* fallback)
{
    const char* value = std::getenv(name);
    return value && value[0] != '\0' ? value : fallback;
}

bool readValue(const std::string& path, int& value, std::string& error)
{
    errno = 0;
    std::ifstream input(path);
    if (!input) {
        error = "open " + path + " for reading: " + std::strerror(errno);
        return false;
    }
    input >> value;
    if (!input) {
        error = "read integer from " + path + " failed";
        return false;
    }
    return true;
}

bool writeValue(const std::string& path, int value, std::string& error)
{
    const int fd = ::open(path.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        error = "open " + path + " for writing: " + std::strerror(errno);
        return false;
    }

    const std::string text = std::to_string(value) + "\n";
    const ssize_t written  = ::write(fd, text.data(), text.size());
    const int write_errno  = errno;
    const int close_result = ::close(fd);
    if (written != static_cast<ssize_t>(text.size())) {
        error = "write " + path + ": " + std::strerror(write_errno);
        return false;
    }
    if (close_result < 0) {
        error = "close " + path + ": " + std::strerror(errno);
        return false;
    }
    return true;
}

}  // namespace

Ext5vPower::Ext5vPower()
    : _path(envOrDefault("CAP_GPS_EXT5V_PATH", kDefaultBrightnessPath)),
      _managed(envEnabled("CAP_GPS_MANAGE_POWER", true))
{
}

Ext5vPower::~Ext5vPower()
{
    disable();
}

bool Ext5vPower::enable(std::string& error)
{
    if (_enabled) {
        error.clear();
        return true;
    }
    error.clear();

    if (!_managed) {
        spdlog::warn("GPS power: EXT5V management disabled by CAP_GPS_MANAGE_POWER");
        _enabled = true;
        return true;
    }

    if (!readValue(_path, _previous_value, error)) {
        error += "; Cap LoRa-1262 requires the EXT5V LED-class interface";
        return false;
    }
    spdlog::info("GPS power: EXT5V interface {} reports brightness={}", _path, _previous_value);
    if (_previous_value <= 0) {
        if (!writeValue(_path, 1, error)) {
            error += "; install the package sudo rule or pre-enable EXT5V";
            return false;
        }
        _restore_needed = true;
        int readback    = 0;
        std::string readback_error;
        if (!readValue(_path, readback, readback_error)) {
            spdlog::warn("GPS power: EXT5V write succeeded but readback failed: {}", readback_error);
        } else if (readback <= 0) {
            spdlog::warn("GPS power: EXT5V write succeeded but brightness still reports 0; continuing to UART probe");
        } else {
            spdlog::info("GPS power: EXT5V enabled (brightness={})", readback);
        }
    } else {
        spdlog::info("GPS power: EXT5V was already enabled; preserving system-owned state");
    }

    _enabled = true;
    return true;
}

void Ext5vPower::disable() noexcept
{
    if (_managed && _restore_needed) {
        std::string error;
        if (!writeValue(_path, _previous_value, error)) {
            spdlog::warn("GPS power: failed to restore EXT5V state: {}", error);
        } else {
            spdlog::info("GPS power: restored EXT5V brightness={}", _previous_value);
        }
    }
    _restore_needed = false;
    _enabled        = false;
}

}  // namespace cap_gps::hal
