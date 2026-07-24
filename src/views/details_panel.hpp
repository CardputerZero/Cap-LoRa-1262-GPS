#pragma once

#include "gps/gps_types.hpp"
#include "views/ui_primitives.hpp"

#include <core/animation/animate_value/animate_value.hpp>
#include <memory>

namespace cap_gps {

class DetailsPanel {
public:
    explicit DetailsPanel(lv_obj_t* parent);
    void setStatus(const gps::GpsStatus& status);
    void scrollBy(int32_t amount);
    void tick(uint32_t nowMs);
    void setOpacity(lv_opa_t opacity);
    void setHidden(bool hidden);
    void setScrollbarHidden(bool hidden);

private:
    static constexpr int32_t kViewportY      = 30;
    static constexpr int32_t kViewportHeight = 123;
    static constexpr int32_t kContentHeight  = 231;

    std::unique_ptr<ui::Panel> _root;
    std::unique_ptr<ui::TextLabel> _title;
    std::unique_ptr<ui::StatusBadge> _status_badge;
    std::unique_ptr<ui::Panel> _viewport;
    std::unique_ptr<ui::Panel> _content;
    std::unique_ptr<ui::InfoRow> _fix;
    std::unique_ptr<ui::InfoRow> _satellites;
    std::unique_ptr<ui::InfoRow> _altitude;
    std::unique_ptr<ui::InfoRow> _speed;
    std::unique_ptr<ui::InfoRow> _course;
    std::unique_ptr<ui::InfoRow> _hdop;
    std::unique_ptr<ui::InfoRow> _utc;
    std::unique_ptr<ui::InfoRow> _uart;
    std::unique_ptr<ui::InfoRow> _sentences;
    std::unique_ptr<ui::InfoRow> _age;
    std::unique_ptr<ui::ScrollBar> _scrollbar;
    smooth_ui_toolkit::AnimateValue _scroll_y{0.0F};
    float _scroll_target = 0.0F;

    void applyScroll();
};

}  // namespace cap_gps
