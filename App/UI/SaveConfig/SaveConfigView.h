#pragma once

#include "SaveConfigModel.h"
#include "UI/Resource/ResourcePool.h"
#include "I18n.h"
#include "lvgl.h"

namespace page {
class SaveConfigView {
  public:
    void create(lv_obj_t* root);
    void destroy();
    void stop() const;
    void appear();

    bool bind(lv_subject_t* subject);
    void unbind();

    void update(const SaveConfigState& state);
    void update(const DataProc::SystemState& state) {
        SaveConfigState s{};
        s.shutting_down = state.shutting_down;
        s.power_cause = state.power_cause;
        s.power_action = state.power_action;
        s.power_pending = state.power_pending;
        s.power_token = state.power_token;
        update(s);
    }

    lv_anim_t* progress_animation() {
        return &progress_animation_;
    }

  private:
    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;

    static void set_progress(void* obj, int32_t progress);
    static void rotate_spinner(void* obj, int32_t angle);
    lv_obj_t* ring_ = nullptr;
    lv_obj_t* icon_ = nullptr;
    lv_obj_t* action_ = nullptr;
    lv_obj_t* column_ = nullptr;
    lv_obj_t* headline_ = nullptr;
    lv_obj_t* subtitle_ = nullptr;
    lv_obj_t* seconds_ = nullptr;
    lv_obj_t* shutdown_icon_ = nullptr;
    bool shutting_down_ = false;
    lv_anim_t progress_animation_{};
};
} // namespace page
