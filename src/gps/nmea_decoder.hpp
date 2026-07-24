#pragma once

#include "gps/gps_types.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace cap_gps::gps {

class NmeaDecoder {
public:
    void reset();
    bool feed(const uint8_t* data, std::size_t size, uint32_t nowMs);

    const NavigationData& navigation() const noexcept
    {
        return _navigation;
    }

    bool hasSentence() const noexcept
    {
        return _has_sentence;
    }

    bool hasFixTimestamp() const noexcept
    {
        return _has_fix_timestamp;
    }

    uint32_t lastSentenceMs() const noexcept
    {
        return _last_sentence_ms;
    }

    uint32_t lastFixMs() const noexcept
    {
        return _last_fix_ms;
    }

private:
    NavigationData _navigation;
    std::string _line;
    bool _discard_line         = false;
    bool _has_sentence         = false;
    bool _has_fix_timestamp    = false;
    uint32_t _last_sentence_ms = 0;
    uint32_t _last_fix_ms      = 0;

    bool finishLine(uint32_t nowMs);
    void updateFixState(bool fixed, uint32_t nowMs);
};

}  // namespace cap_gps::gps
