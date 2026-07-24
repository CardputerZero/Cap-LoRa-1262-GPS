#include "gps/gps_types.hpp"

namespace cap_gps::gps {

const char* gpsStateName(GpsState state)
{
    switch (state) {
        case GpsState::Initializing:
            return "initializing";
        case GpsState::NoData:
            return "no data";
        case GpsState::Searching:
            return "searching";
        case GpsState::Fixed:
            return "fixed";
        case GpsState::Stale:
            return "stale";
        case GpsState::Error:
            return "error";
        case GpsState::Stopped:
            return "stopped";
    }
    return "unknown";
}

}  // namespace cap_gps::gps
