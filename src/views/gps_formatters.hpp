#pragma once

#include "gps/gps_types.hpp"

#include <string>

namespace cap_gps::view_format {

struct BadgeStyle {
    const char* text;
    uint32_t color;
};

BadgeStyle badgeFor(gps::GpsState state, int fixType);
std::string coordinate(double value, bool latitude);
std::string altitude(const gps::NavigationData& data);
std::string speed(const gps::NavigationData& data);
std::string course(const gps::NavigationData& data);
std::string dilution(bool valid, double value);
std::string utc(const gps::UtcDateTime& value);
std::string age(uint32_t milliseconds);

}  // namespace cap_gps::view_format
