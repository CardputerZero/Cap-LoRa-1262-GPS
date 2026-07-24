#pragma once

#include <cstdint>

namespace cap_gps {

class ViewModel {
public:
    virtual ~ViewModel() = default;
    virtual void onEnter()
    {
    }
    virtual void onExit()
    {
    }
    virtual void onKey(uint32_t key) = 0;
    virtual void tick(uint32_t nowMs)
    {
        (void)nowMs;
    }
};

}  // namespace cap_gps
