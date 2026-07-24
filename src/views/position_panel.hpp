#pragma once

#include "gps/gps_types.hpp"
#include "views/ui_primitives.hpp"

#include <memory>

namespace cap_gps {

class PositionPanel {
public:
    explicit PositionPanel(lv_obj_t* parent);
    void setStatus(const gps::GpsStatus& status);
    void setOpacity(lv_opa_t opacity);
    void setHidden(bool hidden);

private:
    std::unique_ptr<ui::Panel> _root;
    std::unique_ptr<ui::TextLabel> _title;
    std::unique_ptr<ui::StatusBadge> _status_badge;
    std::unique_ptr<ui::TextLabel> _latitude_caption;
    std::unique_ptr<ui::TextLabel> _latitude;
    std::unique_ptr<ui::TextLabel> _longitude_caption;
    std::unique_ptr<ui::TextLabel> _longitude;
    std::unique_ptr<ui::Panel> _divider;
    std::unique_ptr<ui::TextLabel> _satellites_caption;
    std::unique_ptr<ui::TextLabel> _satellites;
    std::unique_ptr<ui::TextLabel> _altitude_caption;
    std::unique_ptr<ui::TextLabel> _altitude;
    std::unique_ptr<ui::TextLabel> _speed_caption;
    std::unique_ptr<ui::TextLabel> _speed;
};

}  // namespace cap_gps
