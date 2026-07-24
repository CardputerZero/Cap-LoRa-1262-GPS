#include "gps/gps_backend.hpp"

namespace cap_gps::gps {

#if CAP_GPS_USE_MOCK_BACKEND
std::unique_ptr<GpsBackend> makeMockGpsBackend();
#elif CAP_GPS_USE_LINUX_BACKEND
std::unique_ptr<GpsBackend> makeLinuxSerialBackend();
#endif

std::unique_ptr<GpsBackend> makeGpsBackend()
{
#if CAP_GPS_USE_MOCK_BACKEND
    return makeMockGpsBackend();
#elif CAP_GPS_USE_LINUX_BACKEND
    return makeLinuxSerialBackend();
#else
    return nullptr;
#endif
}

}  // namespace cap_gps::gps
