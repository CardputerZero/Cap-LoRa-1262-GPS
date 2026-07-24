#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace cap_gps::gps {

enum class GpsState {
    Initializing,
    NoData,
    Searching,
    Fixed,
    Stale,
    Error,
    Stopped,
};

struct UtcDateTime {
    bool dateValid = false;
    bool timeValid = false;
    int year       = 0;
    int month      = 0;
    int day        = 0;
    int hour       = 0;
    int minute     = 0;
    int second     = 0;
};

struct NavigationData {
    bool positionValid  = false;
    bool fixValid       = false;
    double latitudeDeg  = 0.0;
    double longitudeDeg = 0.0;

    bool altitudeValid = false;
    double altitudeM   = 0.0;
    bool speedValid    = false;
    double speedKph    = 0.0;
    bool courseValid   = false;
    double courseDeg   = 0.0;
    bool hdopValid     = false;
    double hdop        = 0.0;
    bool pdopValid     = false;
    double pdop        = 0.0;
    bool vdopValid     = false;
    double vdop        = 0.0;

    int fixQuality        = 0;
    int fixType           = 1;
    int satellitesUsed    = 0;
    int satellitesVisible = 0;
    UtcDateTime utc;

    std::string talker;
    std::string lastSentence;
    uint64_t bytesReceived     = 0;
    uint64_t sentencesReceived = 0;
    uint64_t invalidSentences  = 0;
};

struct ReceiverInfo {
    std::string backend;
    std::string device;
    std::string module{"ATGM336H-6N / AT6668"};
    std::string protocol{"NMEA 0183 4.1"};
    int baud  = 115200;
    bool mock = false;
};

struct GpsStatus {
    GpsState state            = GpsState::Stopped;
    bool ready                = false;
    bool initializationFailed = false;
    uint32_t attempt          = 0;
    NavigationData data;
    ReceiverInfo info;
    uint32_t dataAgeMs = std::numeric_limits<uint32_t>::max();
    uint32_t fixAgeMs  = std::numeric_limits<uint32_t>::max();
    std::string diagnostics;
    std::string error;
};

const char* gpsStateName(GpsState state);

}  // namespace cap_gps::gps
