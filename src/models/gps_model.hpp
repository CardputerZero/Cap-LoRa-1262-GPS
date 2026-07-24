#pragma once

#include "gps/gps_backend.hpp"
#include "gps/nmea_decoder.hpp"

#include <memory>
#include <tools/observable/single_observable.hpp>

namespace cap_gps {

class GpsModel {
public:
    GpsModel();
    explicit GpsModel(std::unique_ptr<gps::GpsBackend> backend);
    ~GpsModel();

    GpsModel(const GpsModel&)            = delete;
    GpsModel& operator=(const GpsModel&) = delete;

    void start();
    void stop();
    void tick(uint32_t nowMs);
    bool retry();

    smooth_ui_toolkit::SingleObservable<gps::GpsStatus>& status()
    {
        return _status;
    }

private:
    std::unique_ptr<gps::GpsBackend> _backend;
    gps::NmeaDecoder _decoder;
    smooth_ui_toolkit::SingleObservable<gps::GpsStatus> _status{gps::GpsStatus{}};
    uint32_t _attempt         = 0;
    uint32_t _opened_at_ms    = 0;
    uint32_t _last_publish_ms = 0;
    bool _clock_initialized   = false;
    bool _started             = false;

    bool openBackend();
    void updateStatus(uint32_t nowMs);
    void setError(std::string message, bool initializationFailed);
};

}  // namespace cap_gps
