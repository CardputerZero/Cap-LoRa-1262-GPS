#include "views/details_panel.hpp"

#include "views/gps_formatters.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace cap_gps {

DetailsPanel::DetailsPanel(lv_obj_t* parent)
    : _root(std::make_unique<ui::Panel>(parent, ui::Frame{0, 0, 320, 170}, 0x101214, LV_OPA_COVER)),
      _title(std::make_unique<ui::TextLabel>(_root->raw_ptr(), "DETAILS", ui::Frame{12, 8, 92, 18},
                                             &lv_font_montserrat_14, 0xE4E4E4)),
      _status_badge(std::make_unique<ui::StatusBadge>(_root->raw_ptr(), "STARTING", 0xFED40D)),
      _viewport(std::make_unique<ui::Panel>(_root->raw_ptr(), ui::Frame{0, kViewportY, 320, kViewportHeight})),
      _content(std::make_unique<ui::Panel>(_viewport->raw_ptr(), ui::Frame{0, 0, 320, kContentHeight})),
      _fix(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 4, "FIX")),
      _satellites(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 27, "SATELLITES")),
      _altitude(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 50, "ALTITUDE")),
      _speed(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 73, "SPEED")),
      _course(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 96, "COURSE")),
      _hdop(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 119, "HDOP / PDOP")),
      _utc(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 142, "UTC")),
      _uart(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 165, "UART")),
      _sentences(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 188, "NMEA")),
      _age(std::make_unique<ui::InfoRow>(_content->raw_ptr(), 211, "DATA AGE")),
      _scrollbar(
          std::make_unique<ui::ScrollBar>(_root->raw_ptr(), ui::Frame{314, kViewportY + 4, 3, kViewportHeight - 8}))
{
    _scroll_y.springOptions().visualDuration = 0.38F;
    _scroll_y.springOptions().bounce         = 0.08F;
    applyScroll();
}

void DetailsPanel::setStatus(const gps::GpsStatus& status)
{
    const auto badge = view_format::badgeFor(status.state, status.data.fixType);
    _status_badge->setText(badge.text);
    _status_badge->setColor(badge.color);

    _fix->setValue(badge.text);
    _fix->setValueColor(badge.color);

    char satellites[40] = {};
    std::snprintf(satellites, sizeof(satellites), "%d used / %d visible", status.data.satellitesUsed,
                  status.data.satellitesVisible);
    _satellites->setValue(satellites);
    _altitude->setValue(view_format::altitude(status.data));
    _speed->setValue(view_format::speed(status.data));
    _course->setValue(view_format::course(status.data));
    _hdop->setValue(view_format::dilution(status.data.hdopValid, status.data.hdop) + " / " +
                    view_format::dilution(status.data.pdopValid, status.data.pdop));
    _utc->setValue(view_format::utc(status.data.utc));
    _uart->setValue(status.info.device.empty() ? "--" : status.info.device + " @ " + std::to_string(status.info.baud));

    char sentences[48] = {};
    std::snprintf(sentences, sizeof(sentences), "%llu valid / %llu invalid",
                  static_cast<unsigned long long>(status.data.sentencesReceived),
                  static_cast<unsigned long long>(status.data.invalidSentences));
    _sentences->setValue(sentences);
    _age->setValue(view_format::age(status.dataAgeMs));
}

void DetailsPanel::scrollBy(int32_t amount)
{
    const float maximum = static_cast<float>(std::max(0, kContentHeight - kViewportHeight));
    _scroll_y.update();
    _scroll_target = std::clamp(_scroll_target + static_cast<float>(amount), 0.0F, maximum);
    _scroll_y.move(_scroll_target);
}

void DetailsPanel::tick(uint32_t nowMs)
{
    if (!_scroll_y.done()) {
        _scroll_y.update(static_cast<float>(nowMs) / 1000.0F);
        applyScroll();
    }
}

void DetailsPanel::setOpacity(lv_opa_t opacity)
{
    _root->setOpa(opacity);
}

void DetailsPanel::setHidden(bool hidden)
{
    _root->setHidden(hidden);
}

void DetailsPanel::setScrollbarHidden(bool hidden)
{
    _scrollbar->setHidden(hidden);
}

void DetailsPanel::applyScroll()
{
    const float offset =
        std::clamp(_scroll_y.directValue(), 0.0F, static_cast<float>(std::max(0, kContentHeight - kViewportHeight)));
    _content->setY(-static_cast<int32_t>(std::lround(offset)));
    _scrollbar->update(offset, kViewportHeight, kContentHeight);
}

}  // namespace cap_gps
