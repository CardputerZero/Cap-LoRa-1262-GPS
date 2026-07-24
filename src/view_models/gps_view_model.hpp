#pragma once

#include "core/gps_types.hpp"
#include "models/gps_model.hpp"
#include "view_models/view_model.hpp"

#include <tools/observable/single_observable.hpp>

namespace cap_gps {

class GpsViewModel final : public ViewModel {
public:
    explicit GpsViewModel(GpsModel& model);
    ~GpsViewModel() override;

    void onEnter() override;
    void onExit() override;
    void onKey(uint32_t key) override;

    smooth_ui_toolkit::SingleObservable<gps::GpsStatus>& status()
    {
        return _status;
    }

    smooth_ui_toolkit::SingleObservable<GpsSection>& section()
    {
        return _section;
    }

    smooth_ui_toolkit::SingleObservable<ScrollRequest>& scrollRequest()
    {
        return _scroll_request;
    }

    smooth_ui_toolkit::SingleObservable<bool>& errorDialogActive()
    {
        return _error_dialog_active;
    }

    bool modalActive() const
    {
        return _error_dialog_active.get();
    }

    void dismissErrorDialog();
    void retryReceiver();

private:
    GpsModel& _model;
    smooth_ui_toolkit::SingleObservable<gps::GpsStatus> _status;
    smooth_ui_toolkit::SingleObservable<GpsSection> _section{GpsSection::Position};
    smooth_ui_toolkit::SingleObservable<ScrollRequest> _scroll_request{ScrollRequest{}};
    smooth_ui_toolkit::SingleObservable<bool> _error_dialog_active{false};
    bool _entered = false;

    void moveSection(int direction);
    void requestScroll(int32_t amount);
    static void onStatusChanged(void* context, const gps::GpsStatus& status);
};

}  // namespace cap_gps
