#include "views/gps_view.hpp"

#include "views/details_panel.hpp"
#include "views/position_panel.hpp"
#include "views/ui_primitives.hpp"

#include <core/animation/animate_value/animate_value.hpp>
#include <core/easing/ease.hpp>
#include <spdlog/spdlog.h>

#include <utility>

namespace cap_gps {
namespace {

constexpr float kPageFadeDuration = 0.15F;

void configureFade(smooth_ui_toolkit::AnimateValue& opacity)
{
    opacity.easingOptions().duration       = kPageFadeDuration;
    opacity.easingOptions().easingFunction = smooth_ui_toolkit::ease::ease_in_out_quad;
}

}  // namespace

class GpsView::Pager {
public:
    Pager(lv_obj_t* parent, GpsViewModel& viewModel, bool showTitle)
        : _position(std::make_unique<PositionPanel>(parent)),
          _details(std::make_unique<DetailsPanel>(parent)),
          _indicator(std::make_unique<ui::PageIndicator>(parent)),
          _title(std::make_unique<ui::TitleHud>(parent)),
          _dialog(std::make_unique<ui::InitializationDialog>(
              parent, [&viewModel]() { viewModel.dismissErrorDialog(); },
              [&viewModel]() { viewModel.retryReceiver(); })),
          _position_opacity(255.0F),
          _details_opacity(0.0F)
    {
        configureFade(_position_opacity);
        configureFade(_details_opacity);
        _position->setOpacity(LV_OPA_COVER);
        _position->setHidden(false);
        _details->setOpacity(LV_OPA_TRANSP);
        _details->setHidden(true);
        if (showTitle) {
            _title->show();
        }
    }

    void setStatus(const gps::GpsStatus& status)
    {
        _position->setStatus(status);
        _details->setStatus(status);
        _dialog->setRetryEnabled(status.state == gps::GpsState::Error);
    }

    void setSection(GpsSection section)
    {
        if (!_section_initialized) {
            _section_initialized = true;
            _section             = section;
            _position_opacity.teleport(section == GpsSection::Position ? 255.0F : 0.0F);
            _details_opacity.teleport(section == GpsSection::Details ? 255.0F : 0.0F);
            applyOpacity();
            _position->setHidden(section != GpsSection::Position);
            _details->setHidden(section != GpsSection::Details);
            _indicator->setSection(section);
            return;
        }
        if (section == _section || section == GpsSection::Count) {
            return;
        }

        _position_opacity.update();
        _details_opacity.update();
        applyOpacity();
        _section = section;
        if (_section != GpsSection::Position) {
            _title->dismiss();
        }
        _position->setHidden(false);
        _details->setHidden(false);
        _position_opacity.move(section == GpsSection::Position ? 255.0F : 0.0F);
        _details_opacity.move(section == GpsSection::Details ? 255.0F : 0.0F);
        _indicator->setSection(section);
    }

    void scroll(int32_t amount)
    {
        if (_section == GpsSection::Details) {
            _details->scrollBy(amount);
        }
    }

    void setErrorDialogActive(bool active)
    {
        _dialog_active = active;
        if (active) {
            _title->dismiss();
            _indicator->setHidden(true);
            _details->setScrollbarHidden(true);
        }
        _dialog->setActive(active);
    }

    void tick(uint32_t nowMs)
    {
        _details->tick(nowMs);
        _title->tick(nowMs);
        _dialog->tick(nowMs);

        const float nowSeconds = static_cast<float>(nowMs) / 1000.0F;
        _position_opacity.update(nowSeconds);
        _details_opacity.update(nowSeconds);
        applyOpacity();

        if (!_dialog_active && _dialog->hidden()) {
            _indicator->setHidden(false);
            _details->setScrollbarHidden(false);
        }
        if (_section != GpsSection::Position && _position_opacity.done() && _position_opacity.directValue() <= 0.0F) {
            _position->setHidden(true);
        }
        if (_section != GpsSection::Details && _details_opacity.done() && _details_opacity.directValue() <= 0.0F) {
            _details->setHidden(true);
        }
    }

private:
    std::unique_ptr<PositionPanel> _position;
    std::unique_ptr<DetailsPanel> _details;
    std::unique_ptr<ui::PageIndicator> _indicator;
    std::unique_ptr<ui::TitleHud> _title;
    std::unique_ptr<ui::InitializationDialog> _dialog;
    smooth_ui_toolkit::AnimateValue _position_opacity;
    smooth_ui_toolkit::AnimateValue _details_opacity;
    GpsSection _section       = GpsSection::Position;
    bool _section_initialized = false;
    bool _dialog_active       = false;

    void applyOpacity()
    {
        _position->setOpacity(ui::toOpacity(_position_opacity.directValue()));
        _details->setOpacity(ui::toOpacity(_details_opacity.directValue()));
    }
};

GpsView::GpsView(GpsViewModel& viewModel) : _view_model(viewModel)
{
}

GpsView::~GpsView()
{
    onExit();
}

void GpsView::onEnter(lv_obj_t* parent)
{
    onExit();
    _root = std::make_unique<smooth_ui_toolkit::lvgl_cpp::Container>(parent);
    _root->setSize(320, 170);
    _root->setPos(0, 0);
    _root->setBgColor(lv_color_hex(0x101214));
    _root->setBgOpa(LV_OPA_COVER);
    _root->setBorderWidth(0);
    _root->setShadowWidth(0);
    _root->setPaddingAll(0);
    _root->setScrollbarMode(LV_SCROLLBAR_MODE_OFF);
    _root->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    const bool showTitle = !_title_shown;
    _title_shown         = true;
    _pager               = std::make_unique<Pager>(_root->raw_ptr(), _view_model, showTitle);
    _scroll_serial_seen  = _view_model.scrollRequest().get().serial;

    _view_model.status().observe(this, onStatusChanged);
    _view_model.section().observe(this, onSectionChanged);
    _view_model.scrollRequest().observe(this, onScrollRequestChanged);
    _view_model.errorDialogActive().observe(this, onErrorDialogActiveChanged);
    spdlog::info("GpsView: enter");
}

void GpsView::onExit()
{
    _view_model.errorDialogActive().removeObserver();
    _view_model.scrollRequest().removeObserver();
    _view_model.section().removeObserver();
    _view_model.status().removeObserver();
    _pager.reset();
    _root.reset();
}

void GpsView::tick(uint32_t nowMs)
{
    if (_pager) {
        _pager->tick(nowMs);
    }
}

void GpsView::onStatusChanged(void* context, const gps::GpsStatus& status)
{
    auto* self = static_cast<GpsView*>(context);
    if (self && self->_pager) {
        self->_pager->setStatus(status);
    }
}

void GpsView::onSectionChanged(void* context, const GpsSection& section)
{
    auto* self = static_cast<GpsView*>(context);
    if (self && self->_pager) {
        self->_pager->setSection(section);
    }
}

void GpsView::onScrollRequestChanged(void* context, const ScrollRequest& request)
{
    auto* self = static_cast<GpsView*>(context);
    if (!self || !self->_pager || request.serial == 0 || request.serial == self->_scroll_serial_seen) {
        return;
    }
    self->_scroll_serial_seen = request.serial;
    self->_pager->scroll(request.amount);
}

void GpsView::onErrorDialogActiveChanged(void* context, const bool& active)
{
    auto* self = static_cast<GpsView*>(context);
    if (self && self->_pager) {
        self->_pager->setErrorDialogActive(active);
    }
}

}  // namespace cap_gps
