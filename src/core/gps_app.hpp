#pragma once

#include "models/gps_model.hpp"
#include "view_models/gps_view_model.hpp"
#include "views/gps_view.hpp"

#include <lvgl.h>

namespace cap_gps {

class GpsApp {
public:
    GpsApp();
    ~GpsApp();

    GpsApp(const GpsApp&)            = delete;
    GpsApp& operator=(const GpsApp&) = delete;

    void start();
    void stop();
    void onKey(uint32_t key);
    bool onLvglKeyState(uint32_t lvKey, const char* utf8, bool pressed);
    void tick(uint32_t nowMs);

    bool quitRequested() const
    {
        return _quit_requested;
    }

private:
    GpsModel _model;
    GpsViewModel _view_model;
    GpsView _view;
    lv_group_t* _input_group = nullptr;
    bool _quit_requested     = false;
    bool _started            = false;

    void setupInputGroup();
    static void onKeyboardEvent(lv_event_t* event);
};

}  // namespace cap_gps
