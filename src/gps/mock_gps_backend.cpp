#include "gps/gps_backend.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace cap_gps::gps {
namespace {

std::string sentence(const std::string& body)
{
    uint8_t checksum = 0;
    for (char character : body) {
        checksum ^= static_cast<uint8_t>(character);
    }
    char suffix[8] = {};
    std::snprintf(suffix, sizeof(suffix), "*%02X\r\n", static_cast<unsigned int>(checksum));
    return "$" + body + suffix;
}

class MockGpsBackend final : public GpsBackend {
public:
    bool open(ReceiverInfo& info, std::string& error) override
    {
        ++_open_attempt;
        const char* failCountText = std::getenv("CAP_GPS_MOCK_INIT_FAIL_COUNT");
        char* end                 = nullptr;
        const long failCount      = failCountText ? std::strtol(failCountText, &end, 10) : 0;
        if (failCountText && end != failCountText && failCount > 0 &&
            _open_attempt <= static_cast<unsigned long>(failCount)) {
            _open = false;
            error = "mock GPS initialization failure requested by CAP_GPS_MOCK_INIT_FAIL_COUNT";
            return false;
        }

        error.clear();
        _open  = true;
        _cycle = 0;
        _pending.clear();
        _last_emit   = std::chrono::steady_clock::now() - std::chrono::seconds(1);
        info.backend = "SDL mock GPS";
        info.device  = "mock://atgm336h";
        info.baud    = 115200;
        info.mock    = true;
        return true;
    }

    ReadResult readAvailable(uint8_t* data, std::size_t capacity) override
    {
        if (!_open) {
            return {0, "mock GPS is not open"};
        }
        if (_pending.empty() && std::chrono::steady_clock::now() - _last_emit >= std::chrono::milliseconds(900)) {
            emitCycle();
        }
        if (_pending.empty() || !data || capacity == 0) {
            return {};
        }

        const std::size_t count = std::min(capacity, _pending.size());
        std::copy_n(reinterpret_cast<const uint8_t*>(_pending.data()), count, data);
        _pending.erase(0, count);
        return {count, {}};
    }

    void close() noexcept override
    {
        _open = false;
        _pending.clear();
    }

    bool isOpen() const noexcept override
    {
        return _open;
    }

private:
    bool _open                  = false;
    unsigned int _cycle         = 0;
    unsigned long _open_attempt = 0;
    std::string _pending;
    std::chrono::steady_clock::time_point _last_emit{};

    void emitCycle()
    {
        _last_emit       = std::chrono::steady_clock::now();
        const int second = static_cast<int>(_cycle % 60);
        char time[16]    = {};
        std::snprintf(time, sizeof(time), "0800%02d.00", second);

        if (_cycle < 2) {
            _pending += sentence(std::string("GNRMC,") + time + ",V,,,,,,,230726,,,N");
            _pending += sentence(std::string("GNGGA,") + time + ",,,,,0,00,99.99,,,,,,");
            _pending += sentence("GNGSV,1,1,04,03,42,112,24,07,31,231,18,08,66,047,27,11,19,301,15");
        } else {
            _pending += sentence(std::string("GNRMC,") + time + ",A,2232.5858,N,11403.4719,E,1.25,73.4,230726,,,A");
            _pending += sentence(std::string("GNGGA,") + time + ",2232.5858,N,11403.4719,E,1,12,0.82,35.6,M,-2.3,M,,");
            _pending += sentence("GNGSA,A,3,03,07,08,11,16,20,22,26,,,,,1.40,0.82,1.13");
            _pending += sentence("GNGSV,1,1,18,03,42,112,37,07,31,231,34,08,66,047,41,11,19,301,28");
            _pending += sentence("GNVTG,73.4,T,,M,1.25,N,2.32,K,A");
        }
        ++_cycle;
    }
};

}  // namespace

std::unique_ptr<GpsBackend> makeMockGpsBackend()
{
    return std::make_unique<MockGpsBackend>();
}

}  // namespace cap_gps::gps
