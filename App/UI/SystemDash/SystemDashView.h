#pragma once
#include <array>
#include "SystemDashModel.h"
#include "UI/Resource/ResourcePool.h"
#include "lvgl.h"
#include "UI/Common/FocusHighlight.h"

namespace page {
class SystemDashView {
  public:
    enum class Control { Language, Wifi, Power, Back, Shutdown, Reboot, None };

    void create(lv_obj_t* root, bool reboot_available = true);
    void destroy();
    void appear() const;
    void stop();
    /** @brief 组装或恢复焦点后直接落位，再启用后续保速移动。 */
    void resume_focus();
    /** @brief 布局或语言更新时直接对齐当前焦点。 */
    void sync_focus();
    lv_obj_t* focus_highlight() const { return focus_.object(); }
    const app::LvSpring& focus_motion() const { return focus_.motion(); }

    bool bind(lv_subject_t* subject);
    void unbind();

    void apply_language();
    void update(const SystemDashState& state) const;
    void show_power(bool visible);
    /** Hold-to-confirm progress 0..100 inside button (shutdown or reboot); 0 clears both. */
    void set_hold(lv_obj_t* button, int32_t percent) const;
    bool power_open() const {
        return power_open_;
    }

    Control control(const lv_obj_t* object) const;
    lv_obj_t* control_object(Control control) const;
    bool is_reboot_action(const lv_obj_t* object) const { return object == ui.reboot; }
    lv_obj_t* default_focus(bool power_dialog) const { return power_dialog ? ui.shutdown : ui.power; }
    std::array<lv_obj_t*, 2> power_actions() const { return {ui.shutdown, reboot_available_ ? ui.reboot : nullptr}; }

    std::array<lv_obj_t*, 4> controls() const;
    std::array<lv_obj_t*, 6> all_controls() const;

    const auto& widgets() const { return ui; }

  private:
    struct {
        lv_obj_t *language{}, *wifi{}, *power{}, *back{}, *overlay{}, *shutdown{}, *reboot{};
    } ui;

    FocusHighlight focus_{};

    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;

    struct Caption {
        lv_obj_t* label;
        i18n::TextId id;
    };
    std::array<Caption, 10> captions_{};
    unsigned caption_count_ = 0;
    bool power_open_ = false, reboot_available_ = true;
    lv_obj_t *language_value_{}, *wifi_value_{}, *hero_{}, *status_dot_{};
    lv_obj_t* hold_fill_[2]{}; // Shutdown, reboot.
    lv_obj_t* caption(lv_obj_t* parent, i18n::TextId id, int x, int y, int width);
};
} // namespace page
