#pragma once

#include <string>

namespace cap_gps::hal {

class Ext5vPower {
public:
    Ext5vPower();
    ~Ext5vPower();

    Ext5vPower(const Ext5vPower&)            = delete;
    Ext5vPower& operator=(const Ext5vPower&) = delete;

    bool enable(std::string& error);
    void disable() noexcept;

    bool enabled() const noexcept
    {
        return _enabled;
    }

    const std::string& path() const noexcept
    {
        return _path;
    }

private:
    std::string _path;
    int _previous_value  = 0;
    bool _restore_needed = false;
    bool _managed        = true;
    bool _enabled        = false;
};

}  // namespace cap_gps::hal
