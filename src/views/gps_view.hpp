#pragma once

#include "view_models/gps_view_model.hpp"
#include "views/view.hpp"

#include <lvgl/lvgl_cpp/obj.hpp>
#include <memory>

namespace cap_gps {

class GpsView final : public View {
public:
    explicit GpsView(GpsViewModel& viewModel);
    ~GpsView() override;

    void onEnter(lv_obj_t* parent) override;
    void onExit() override;
    void tick(uint32_t nowMs) override;

private:
    class Pager;
    GpsViewModel& _view_model;
    std::unique_ptr<smooth_ui_toolkit::lvgl_cpp::Container> _root;
    std::unique_ptr<Pager> _pager;
    uint32_t _scroll_serial_seen = 0;
    bool _title_shown            = false;

    static void onStatusChanged(void* context, const gps::GpsStatus& status);
    static void onSectionChanged(void* context, const GpsSection& section);
    static void onScrollRequestChanged(void* context, const ScrollRequest& request);
    static void onErrorDialogActiveChanged(void* context, const bool& active);
};

}  // namespace cap_gps
