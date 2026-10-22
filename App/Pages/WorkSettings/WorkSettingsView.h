#pragma once
#include <array>
#include "DataProc/DataProc_Def.h"
#include "Resource/ResourcePool.h"

namespace page {
class WorkSettingsView {
  public:
    void create(lv_obj_t* root);
    void destroy() {
        stop();
    }
    void appear() const;
    void stop() const;
    void set_selection(unsigned protocol, unsigned channel);
    void update(DataProc::WorkTarget target, bool pending, i18n::TextId feedback) const;
    void cycle_protocol() {
        protocol_ = (protocol_ + 1) % DataProc::kRadioProtocolCount;
    }
    void cycle_channel() {
        channel_ = (channel_ + 1) % DataProc::kRadioChannelCount;
    }
    unsigned protocol() const {
        return protocol_;
    }
    unsigned channel() const {
        return channel_;
    }
    std::array<lv_obj_t*, 5> controls() const {
        return {ui.preset, ui.protocol, ui.channel, ui.apply, ui.back};
    }
    struct {
        lv_obj_t *preset{}, *protocol{}, *channel{}, *apply{}, *back{};
    } ui;

  private:
    unsigned protocol_ = 0, channel_ = 0;
    lv_obj_t *preset_text_{}, *protocol_text_{}, *channel_text_{}, *apply_text_{}, *feedback_{};
    lv_obj_t *mode_icon_{}, *network_card_{}, *status_dot_{};
};
} // namespace page
