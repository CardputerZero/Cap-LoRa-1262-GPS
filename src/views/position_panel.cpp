#include "views/position_panel.hpp"

#include "views/gps_formatters.hpp"

#include <cstdio>

namespace cap_gps {

PositionPanel::PositionPanel(lv_obj_t* parent)
    : _root(std::make_unique<ui::Panel>(parent, ui::Frame{0, 0, 320, 170}, 0x101214, LV_OPA_COVER)),
      _title(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "POSITION", ui::Frame{12, 8, 92, 18},
                                             &lv_font_montserrat_14, 0xE4E4E4)),
      _status_badge(std::make_unique<ui::StatusBadge>(_root->raw_ptr(), "STARTING", 0xFED40D)),
      _latitude_caption(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "LATITUDE", ui::Frame{14, 34, 90, 14},
                                                        &lv_font_montserrat_10, 0x777B82)),
      _latitude(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "--", ui::Frame{14, 48, 292, 24},
                                                &lv_font_montserrat_20, 0xD9D9D9)),
      _longitude_caption(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "LONGITUDE", ui::Frame{14, 76, 90, 14},
                                                         &lv_font_montserrat_10, 0x777B82)),
      _longitude(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "--", ui::Frame{14, 90, 292, 24},
                                                 &lv_font_montserrat_20, 0xD9D9D9)),
      _divider(std::make_unique<ui::Panel>(_root->raw_ptr(), ui::Frame{12, 119, 296, 1}, 0x34373B, LV_OPA_COVER)),
      _satellites_caption(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "SATELLITES", ui::Frame{14, 126, 88, 13},
                                                          &lv_font_montserrat_10, 0x777B82)),
      _satellites(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "--", ui::Frame{14, 139, 88, 16},
                                                  &lv_font_montserrat_14, 0xE4E4E4)),
      _altitude_caption(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "ALTITUDE", ui::Frame{112, 126, 88, 13},
                                                        &lv_font_montserrat_10, 0x777B82)),
      _altitude(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "--", ui::Frame{112, 139, 88, 16},
                                                &lv_font_montserrat_14, 0xE4E4E4)),
      _speed_caption(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "SPEED", ui::Frame{210, 126, 96, 13},
                                                     &lv_font_montserrat_10, 0x777B82)),
      _speed(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "--", ui::Frame{210, 139, 96, 16},
                                             &lv_font_montserrat_14, 0xE4E4E4))
{
}

void PositionPanel::setStatus(const gps::GpsStatus& status)
{
    const auto badge = view_format::badgeFor(status.state, status.data.fixType);
    _status_badge->setText(badge.text);
    _status_badge->setColor(badge.color);

    if (status.data.positionValid) {
        _latitude->setText(view_format::coordinate(status.data.latitudeDeg, true));
        _longitude->setText(view_format::coordinate(status.data.longitudeDeg, false));
        const uint32_t positionColor = status.state == gps::GpsState::Fixed ? 0xF0F0F0 : 0x94979C;
        _latitude->setTextColor(lv_color_hex(positionColor));
        _longitude->setTextColor(lv_color_hex(positionColor));
    } else {
        _latitude->setText("--");
        _longitude->setText("--");
        _latitude->setTextColor(lv_color_hex(0x707378));
        _longitude->setTextColor(lv_color_hex(0x707378));
    }

    char satellites[24] = {};
    if (status.data.satellitesUsed > 0 || status.data.satellitesVisible > 0) {
        std::snprintf(satellites, sizeof(satellites), "%d / %d", status.data.satellitesUsed,
                      status.data.satellitesVisible);
        _satellites->setText(satellites);
    } else {
        _satellites->setText("--");
    }
    _altitude->setText(view_format::altitude(status.data));
    _speed->setText(view_format::speed(status.data));
}

void PositionPanel::setOpacity(lv_opa_t opacity)
{
    _root->setOpa(opacity);
}

void PositionPanel::setHidden(bool hidden)
{
    _root->setHidden(hidden);
}

}  // namespace cap_gps
