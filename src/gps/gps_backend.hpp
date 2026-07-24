#pragma once

#include "gps/gps_types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace cap_gps::gps {

struct ReadResult {
    std::size_t bytes = 0;
    std::string error;
};

class GpsBackend {
public:
    virtual ~GpsBackend() = default;

    GpsBackend(const GpsBackend&)            = delete;
    GpsBackend& operator=(const GpsBackend&) = delete;

    virtual bool open(ReceiverInfo& info, std::string& error)             = 0;
    virtual ReadResult readAvailable(uint8_t* data, std::size_t capacity) = 0;
    virtual void close() noexcept                                         = 0;
    virtual bool isOpen() const noexcept                                  = 0;

protected:
    GpsBackend() = default;
};

std::unique_ptr<GpsBackend> makeGpsBackend();

}  // namespace cap_gps::gps
