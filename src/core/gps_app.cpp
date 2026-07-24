#include "core/gps_app.hpp"

#include "core/gps_types.hpp"

#include <spdlog/spdlog.h>

namespace cap_gps {
namespace {

bool isTextKey(const char* utf8, char expectedLowercase)
{
    if (!utf8 || utf8[0] == '\0' || utf8[1] != '\0') {
        return false;
    }
    return utf8[0] == expectedLowercase || utf8[0] == expectedLowercase - ('a' - 'A');
}

}  // namespace

GpsApp::GpsApp() : _view_model(_model), _view(_view_model)
{
}

GpsApp::~GpsApp()
{
    stop();
}

void GpsApp::start()
{
    if (_started) {
        return;
    }
    spdlog::info("GpsApp: start");
    _started        = true;
    _quit_requested = false;
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x101214), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_screen_active(), LV_OPA_COVER, LV_PART_MAIN);
    setupInputGroup();
    _view_model.onEnter();
    _view.onEnter(lv_screen_active());
    _model.start();
}

void GpsApp::stop()
{
    if (!_started) {
        return;
    }
    spdlog::info("GpsApp: stopping receiver");
    _model.stop();
    _view.onExit();
    _view_model.onExit();
    if (_input_group) {
#if LV_USE_SDL
        lv_indev_t* inputDevice = lv_indev_get_next(nullptr);
        while (inputDevice) {
            if (lv_indev_get_type(inputDevice) == LV_INDEV_TYPE_KEYPAD) {
                lv_indev_remove_event_cb_with_user_data(inputDevice, onKeyboardEvent, this);
            }
            inputDevice = lv_indev_get_next(inputDevice);
        }
#endif
        lv_group_del(_input_group);
        _input_group = nullptr;
    }
    _started = false;
    spdlog::info("GpsApp: stop complete");
}

void GpsApp::onKey(uint32_t key)
{
    if (key == '\x1b' && !_view_model.modalActive()) {
        spdlog::info("GpsApp: quit requested");
        _quit_requested = true;
        return;
    }
    _view_model.onKey(key);
}

bool GpsApp::onLvglKeyState(uint32_t lvKey, const char* utf8, bool pressed)
{
    if (!pressed) {
        return true;
    }
    switch (lvKey) {
        case LV_KEY_ESC:
            onKey('\x1b');
            return true;
        case LV_KEY_ENTER:
            onKey('\r');
            return true;
        case LV_KEY_UP:
            onKey(gps_key::Up);
            return true;
        case LV_KEY_DOWN:
            onKey(gps_key::Down);
            return true;
        case LV_KEY_LEFT:
        case LV_KEY_PREV:
            onKey(gps_key::Left);
            return true;
        case LV_KEY_RIGHT:
        case LV_KEY_NEXT:
            onKey(gps_key::Right);
            return true;
        default:
            break;
    }
    if (isTextKey(utf8, 'f')) {
        onKey(gps_key::Up);
    } else if (isTextKey(utf8, 'x')) {
        onKey(gps_key::Down);
    } else if (isTextKey(utf8, 'z')) {
        onKey(gps_key::Left);
    } else if (isTextKey(utf8, 'c')) {
        onKey(gps_key::Right);
    }
    return true;
}

void GpsApp::tick(uint32_t nowMs)
{
    _model.tick(nowMs);
    _view_model.tick(nowMs);
    _view.tick(nowMs);
}

void GpsApp::setupInputGroup()
{
    if (_input_group) {
        return;
    }
    _input_group            = lv_group_create();
    lv_indev_t* inputDevice = lv_indev_get_next(nullptr);
    while (inputDevice) {
        if (lv_indev_get_type(inputDevice) == LV_INDEV_TYPE_KEYPAD) {
            lv_indev_set_group(inputDevice, _input_group);
#if LV_USE_SDL
            lv_indev_add_event_cb(inputDevice, onKeyboardEvent, LV_EVENT_KEY, this);
#endif
        }
        inputDevice = lv_indev_get_next(inputDevice);
    }
}

void GpsApp::onKeyboardEvent(lv_event_t* event)
{
    auto* self        = static_cast<GpsApp*>(lv_event_get_user_data(event));
    auto* inputDevice = static_cast<lv_indev_t*>(lv_event_get_target(event));
    if (!self || !inputDevice || lv_indev_get_state(inputDevice) != LV_INDEV_STATE_PRESSED) {
        return;
    }
    const uint32_t key = lv_indev_get_key(inputDevice);
    char utf8[2]       = {0, 0};
    if (key >= 0x20 && key < 0x7f) {
        utf8[0] = static_cast<char>(key);
    }
    self->onLvglKeyState(key, utf8, true);
}

}  // namespace cap_gps
