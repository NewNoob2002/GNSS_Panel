#pragma once
#include <array>
#include "WorkSettingsModel.h"
#include "I18n.h"
#include "UI/Resource/ResourcePool.h"
#include "lvgl.h"

namespace page {
class WorkSettingsView {
  public:
    enum class Control { Preset, Protocol, Channel, Apply, Back, None };

    void create(lv_obj_t* root);
    void destroy() {
        unbind();
        stop();
    }
    void appear() const;
    void stop() const;

    bool bind(lv_subject_t* subject);
    void unbind();

    std::array<lv_obj_t*, 5> controls() const {
        return {ui.preset, ui.protocol, ui.channel, ui.apply, ui.back};
    }
    Control control(const lv_obj_t* object) const;
    lv_obj_t* default_focus(bool pending) const {
        return pending ? ui.back : ui.preset;
    }

    bool update(const WorkSettingsState& state);

    const auto& widgets() const { return ui; }

  private:
    struct {
        lv_obj_t *preset{}, *protocol{}, *channel{}, *apply{}, *back{};
    } ui;

    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;

    lv_obj_t *preset_text_{}, *protocol_text_{}, *channel_text_{}, *apply_text_{}, *feedback_{};
    lv_obj_t *mode_icon_{}, *network_card_{}, *status_dot_{};
};
} // namespace page
