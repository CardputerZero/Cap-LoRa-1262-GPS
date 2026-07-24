#pragma once

#include <cstdint>
#include <lvgl.h>

namespace cap_gps {

class View {
public:
    virtual ~View()                        = default;
    virtual void onEnter(lv_obj_t* parent) = 0;
    virtual void onExit()                  = 0;
    virtual void tick(uint32_t nowMs)
    {
        (void)nowMs;
    }
};

}  // namespace cap_gps
