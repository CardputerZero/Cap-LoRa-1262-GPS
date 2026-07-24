#include "views/gps_formatters.hpp"

#include <cmath>
#include <cstdio>
#include <limits>

namespace cap_gps::view_format {
namespace {

std::string numberWithUnit(double value, int precision, const char* unit)
{
    char text[48] = {};
    std::snprintf(text, sizeof(text), "%.*f%s", precision, value, unit);
    return text;
}

}  // namespace

BadgeStyle badgeFor(gps::GpsState state, int fixType)
{
    switch (state) {
        case gps::GpsState::Initializing:
            return {"STARTING", 0xFED40D};
        case gps::GpsState::NoData:
            return {"NO DATA", 0xE8A84C};
        case gps::GpsState::Searching:
            return {"SEARCHING", 0xFED40D};
        case gps::GpsState::Fixed:
            return {fixType >= 3 ? "3D FIX" : "2D FIX", 0x3FCC75};
        case gps::GpsState::Stale:
            return {"STALE", 0xE8A84C};
        case gps::GpsState::Error:
            return {"ERROR", 0xEF6461};
        case gps::GpsState::Stopped:
            return {"STOPPED", 0x777B82};
    }
    return {"UNKNOWN", 0x777B82};
}

std::string coordinate(double value, bool latitude)
{
    const char direction = latitude ? (value >= 0.0 ? 'N' : 'S') : (value >= 0.0 ? 'E' : 'W');
    char text[40]        = {};
    std::snprintf(text, sizeof(text), "%.6f %c", std::fabs(value), direction);
    return text;
}

std::string altitude(const gps::NavigationData& data)
{
    return data.altitudeValid ? numberWithUnit(data.altitudeM, 1, " m") : "--";
}

std::string speed(const gps::NavigationData& data)
{
    return data.speedValid ? numberWithUnit(data.speedKph, 1, " km/h") : "--";
}

std::string course(const gps::NavigationData& data)
{
    return data.courseValid ? numberWithUnit(data.courseDeg, 1, " deg") : "--";
}

std::string dilution(bool valid, double value)
{
    return valid ? numberWithUnit(value, 2, "") : "--";
}

std::string utc(const gps::UtcDateTime& value)
{
    char text[48] = {};
    if (value.dateValid && value.timeValid) {
        std::snprintf(text, sizeof(text), "%04d-%02d-%02d %02d:%02d:%02d", value.year, value.month, value.day,
                      value.hour, value.minute, value.second);
    } else if (value.timeValid) {
        std::snprintf(text, sizeof(text), "%02d:%02d:%02d UTC", value.hour, value.minute, value.second);
    } else {
        return "--";
    }
    return text;
}

std::string age(uint32_t milliseconds)
{
    if (milliseconds == std::numeric_limits<uint32_t>::max()) {
        return "--";
    }
    if (milliseconds < 1000) {
        return "<1 s";
    }
    char text[24] = {};
    std::snprintf(text, sizeof(text), "%u s", static_cast<unsigned int>(milliseconds / 1000));
    return text;
}

}  // namespace cap_gps::view_format
