#pragma once

#include <cstdint>

namespace cap_gps {

namespace gps_key {

constexpr uint32_t Up    = 0x10001;
constexpr uint32_t Down  = 0x10002;
constexpr uint32_t Left  = 0x10003;
constexpr uint32_t Right = 0x10004;
constexpr uint32_t Help  = 0x10005;

}  // namespace gps_key

enum class GpsSection {
    Position = 0,
    Details,
    Count,
};

struct ScrollRequest {
    uint32_t serial = 0;
    int32_t amount  = 0;
};

constexpr int32_t kDetailsScrollStep = 36;

const char* gpsSectionName(GpsSection section);

}  // namespace cap_gps
