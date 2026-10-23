#pragma once
#include <array>
#include "RecordConfigModel.h"
#include "UI/Resource/ResourcePool.h"
#include "lvgl.h"

namespace page {
class RecordConfigView {
  public:
    enum class Control { Type, Interval, Record, Back, None };

    void create(lv_obj_t* root);
    void destroy() {
        unbind();
        stop();
    }
    void appear() const;
    void stop() const;

    bool bind(lv_subject_t* subject);
    void unbind();

    std::array<lv_obj_t*, 4> controls() const {
        return {ui.type, ui.interval, ui.record, ui.back};
    }
    Control control(const lv_obj_t* object) const;
    lv_obj_t* record_control() const { return ui.record; }

    bool update(const RecordConfigState& state);

    const auto& widgets() const { return ui; }

  private:
    struct {
        lv_obj_t *type{}, *interval{}, *record{}, *back{};
    } ui;

    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;

    lv_obj_t *state_{}, *space_{}, *type_text_{}, *interval_text_{}, *record_text_{}, *back_text_{}, *feedback_{};
    lv_obj_t *hero_{}, *status_dot_{}, *record_icon_{};
};
} // namespace page
