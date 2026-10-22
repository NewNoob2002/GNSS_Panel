#pragma once
#include <array>
#include "DataProc/DataProc_Def.h"
#include "UI/Resource/ResourcePool.h"

namespace page {
class SystemDashView {
  public:
    void create(lv_obj_t* root, bool reboot_available = true);
    void destroy() {
        stop();
    }
    void appear() const;
    void stop() const;
    void apply_language() const;
    void update(const DataProc::SystemState& state) const;
    void show_power(bool visible);
    /** Hold-to-confirm progress 0..100 inside button (shutdown or reboot); 0 clears both. */
    void set_hold(lv_obj_t* button, int32_t percent) const;
    bool power_open() const {
        return power_open_;
    }
    std::array<lv_obj_t*, 4> controls() const;
    std::array<lv_obj_t*, 6> all_controls() const;
    struct {
        lv_obj_t *language{}, *wifi{}, *power{}, *back{}, *overlay{}, *shutdown{}, *reboot{};
    } ui;

  private:
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
