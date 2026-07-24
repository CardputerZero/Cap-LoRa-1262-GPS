#pragma once

#include <lvgl.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace cap_gps {

class GpsKeypad {
public:
    using KeyCallback = std::function<bool(uint32_t, const char*, bool)>;

    GpsKeypad() = default;
    ~GpsKeypad();

    GpsKeypad(const GpsKeypad&)            = delete;
    GpsKeypad& operator=(const GpsKeypad&) = delete;

    bool openDefault();
    bool openDevice(const std::string& path, bool require_app_keys);
    void close();
    void poll();
    lv_indev_t* indev() const;
    void setKeyCallback(KeyCallback callback);

private:
    struct KeyEvent {
        uint32_t key = 0;
        bool pressed = false;
    };

    static void readCb(lv_indev_t* indev, lv_indev_data_t* data);

    bool ensureIndev();
    void pushKeyEvent(uint16_t code, int32_t value);
    uint32_t translateKey(uint16_t code) const;
    const char* keyUtf8(uint32_t key) const;
    bool shiftPressed() const;

    lv_indev_t* _indev = nullptr;
    std::vector<int> _event_fds;
    std::deque<KeyEvent> _pending_keys;
    KeyCallback _key_callback;
    uint32_t _last_key        = 0;
    bool _left_shift_pressed  = false;
    bool _right_shift_pressed = false;
};

}  // namespace cap_gps
