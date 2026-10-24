#ifndef STARTUP_VIEW_H
#define STARTUP_VIEW_H

#include "StartupModel.h"
#include "UI/Resource/ResourcePool.h"
#include "I18n.h"
#include "lvgl.h"

namespace page {
class StartupView {
  public:
    /** Create the standby battery readout and transparent hold-to-start target. */
    void create(lv_obj_t* root);

    /** Stop the charging animation and hold anim before the page is hidden or destroyed. */
    void destroy();

    /** @brief 绑定时立即渲染 Model 自有值；离页解绑，卸载后不触碰控件。 */
    bool bind(lv_subject_t* subject);
    void unbind();

    /** Refresh the split digits/unit readout, battery slot, and version info. */
    void update(const StartupState& state);

    /** Refresh the start and hold labels without changing the hold target. */
    void apply_language() const;

    lv_obj_t* hold_control() const { return ui.btn_press; }
    void reset_hold();
    void set_hold_progress(int32_t value);
    void start_hold_anim(uint32_t hold_ms);
    void reset_hold_anim(uint32_t restore_ms = 300);
    void stop_hold_anim();

    const auto& widgets() const { return ui; }

  private:
    struct {
        lv_obj_t* heading;
        lv_obj_t* arc;
        lv_obj_t* arc_percent;
        lv_anim_t arc_anim;

        lv_obj_t* btn_press;
        lv_obj_t* btn_label;
        lv_obj_t* hold_hint;

        lv_obj_t* bat_caption;
        lv_obj_t* bat_charge;
        lv_obj_t* bat_unit;
        lv_obj_t* bat_voltage;
        lv_obj_t* bat_fill;

        lv_obj_t* firmware;
        lv_obj_t* hardware;
    } ui{};

    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    static void anim_cb(void* var, int32_t value);
    lv_observer_t* observer_ = nullptr;
};
} // namespace page

#endif // STARTUP_VIEW_H
