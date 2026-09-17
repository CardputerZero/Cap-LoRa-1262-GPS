#include "views/help_view.hpp"

namespace cap_gps {

HelpView::HelpView(lv_obj_t* parent)
    : _overlay(std::make_unique<ui::Panel>(parent, ui::Frame{0, 0, 320, 170}, 0x000000, LV_OPA_80)),
      _panel(std::make_unique<ui::Panel>(_overlay->raw_ptr(), ui::Frame{17, 12, 286, 146}, 0x15171A, LV_OPA_COVER, 6)),
      _title(std::make_unique<ui::TextLabel>(_panel->raw_ptr(), "GPS", ui::Frame{14, 10, 258, 20},
                                             &lv_font_montserrat_14, 0xE4E4E4)),
      _body(std::make_unique<ui::TextLabel>(_panel->raw_ptr(),
                                            "Connect Cap LoRa-1262 to read GPS information.\n\n"
                                            "F / X / Z / C: Switch between screens.",
                                            ui::Frame{14, 37, 258, 82}, &lv_font_montserrat_14, 0xD6D9DC)),
      _footer(std::make_unique<ui::TextLabel>(_panel->raw_ptr(), "Fn+H / Esc: close", ui::Frame{14, 122, 258, 16},
                                              &lv_font_montserrat_10, 0x9A9FA5, LV_TEXT_ALIGN_RIGHT))
{
    _overlay->addFlag(LV_OBJ_FLAG_CLICKABLE);
    _panel->setBorderWidth(1);
    _panel->setBorderColor(lv_color_hex(0x55585E));
    _overlay->setHidden(true);
}

HelpView::~HelpView() = default;

void HelpView::show()
{
    _overlay->setHidden(false);
    _overlay->moveForeground();
}

void HelpView::hide()
{
    _overlay->setHidden(true);
}

void HelpView::toggle()
{
    if (visible()) {
        hide();
    } else {
        show();
    }
}

bool HelpView::visible() const
{
    return _overlay && !_overlay->hasFlag(LV_OBJ_FLAG_HIDDEN);
}

}  // namespace cap_gps
