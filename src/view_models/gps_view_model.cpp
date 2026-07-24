#include "view_models/gps_view_model.hpp"

#include <algorithm>

namespace cap_gps {

GpsViewModel::GpsViewModel(GpsModel& model) : _model(model), _status(model.status().get())
{
}

GpsViewModel::~GpsViewModel()
{
    onExit();
}

void GpsViewModel::onEnter()
{
    if (_entered) {
        return;
    }
    _entered = true;
    _model.status().observe(this, onStatusChanged);
}

void GpsViewModel::onExit()
{
    if (!_entered) {
        return;
    }
    _model.status().removeObserver();
    _entered = false;
}

void GpsViewModel::onKey(uint32_t key)
{
    if (_error_dialog_active.get()) {
        if (key == '\x1b') {
            dismissErrorDialog();
        } else if (key == '\r') {
            retryReceiver();
        }
        return;
    }

    if (key == gps_key::Left) {
        moveSection(-1);
    } else if (key == gps_key::Right) {
        moveSection(1);
    } else if (key == gps_key::Up) {
        requestScroll(-kDetailsScrollStep);
    } else if (key == gps_key::Down) {
        requestScroll(kDetailsScrollStep);
    } else if (key == '\r' && _model.status().get().state == gps::GpsState::Error) {
        retryReceiver();
    }
}

void GpsViewModel::dismissErrorDialog()
{
    _error_dialog_active.set(false);
}

void GpsViewModel::retryReceiver()
{
    _error_dialog_active.set(false);
    (void)_model.retry();
}

void GpsViewModel::moveSection(int direction)
{
    const int count   = static_cast<int>(GpsSection::Count);
    const int current = static_cast<int>(_section.get());
    const int next    = std::clamp(current + direction, 0, count - 1);
    if (next != current) {
        _section.set(static_cast<GpsSection>(next));
    }
}

void GpsViewModel::requestScroll(int32_t amount)
{
    if (_section.get() != GpsSection::Details) {
        return;
    }
    auto request = _scroll_request.get();
    ++request.serial;
    request.amount = amount;
    _scroll_request.set(request);
}

void GpsViewModel::onStatusChanged(void* context, const gps::GpsStatus& status)
{
    auto* self = static_cast<GpsViewModel*>(context);
    if (!self) {
        return;
    }
    self->_status.set(status);
    if (status.state == gps::GpsState::Error) {
        self->_error_dialog_active.set(true);
    }
}

}  // namespace cap_gps
