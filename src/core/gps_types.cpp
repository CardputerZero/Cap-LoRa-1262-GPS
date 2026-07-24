#include "core/gps_types.hpp"

namespace cap_gps {

const char* gpsSectionName(GpsSection section)
{
    switch (section) {
        case GpsSection::Position:
            return "position";
        case GpsSection::Details:
            return "details";
        case GpsSection::Count:
            break;
    }
    return "unknown";
}

}  // namespace cap_gps
