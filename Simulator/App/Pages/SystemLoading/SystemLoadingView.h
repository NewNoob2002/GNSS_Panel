#ifndef LVGL_SYSTEM_LOADING_VIEW_H
#define LVGL_SYSTEM_LOADING_VIEW_H

#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "lvgl.h"

namespace page {
/** Simulated boot presentation; Ready is not evidence of initialized hardware. */
class SystemLoadingView {
  public:
    static constexpr uint32_t kTransitionMs = 220;
    static constexpr unsigned kStepCount = 5;
    static constexpr uint32_t kStepMs = 600;
    static constexpr uint32_t kInitializationMs = kStepCount * kStepMs;
    void create(lv_obj_t* root);
    /** UI thread: cancel child animations before hiding/unloading; the page root still owns the widgets. */
    void destroy();
    void show_logo() const;
    void show_initialization();
    /** Switch initialization content for step 0..4 and preserve red failed nodes through completion. */
    void show_step(unsigned step, bool success);
    void show_ready();
    bool has_failures() const {
        return failed_steps_ != 0;
    }
    void apply_language() const;

  private:
    static void set_progress(void* obj, int32_t percent);
    static void set_ready_mix(void* obj, int32_t mix);
    void update_progress(int32_t percent);
    void update_text() const;
    void set_accent(lv_color_t color);
    struct {
        lv_obj_t* cont{};
        lv_obj_t* ring{};
        lv_obj_t* gear{};
        lv_obj_t* check{};
        lv_obj_t* service{};
        lv_obj_t* subtitle{};
        lv_obj_t* bar{};
        lv_obj_t* step{};
        lv_obj_t* percent{};
        lv_obj_t* dots[5]{};
        lv_obj_t* links[4]{};
        lv_obj_t* footer{};
        lv_obj_t* img_logo{};
    } ui_{};
    bool ready_ = false;
    int32_t progress_ = 0;
    int step_ = 1;
    unsigned failed_steps_ = 0;
};
} // namespace page

#endif
