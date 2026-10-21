#pragma once
#include <array>
#include "Common/DataProc/DataProc_Def.h"
#include "Resource/ResourcePool.h"
namespace page {
class RecordConfigView {
  public:
    void create(lv_obj_t* root);
    void destroy() {
        stop();
    }
    void appear() const;
    void stop() const;
    void set_selection(unsigned type, unsigned interval);
    void update(const DataProc::DeviceSnapshot& device, bool configurable, i18n::TextId feedback) const;
    void cycle_type() {
        type_ = (type_ + 1) % DataProc::kRecordTypeCount;
    }
    void cycle_interval() {
        interval_ = (interval_ + 1) % DataProc::kRecordIntervalCount;
    }
    unsigned type() const {
        return type_;
    }
    unsigned interval() const {
        return interval_;
    }
    std::array<lv_obj_t*, 4> controls() const {
        return {ui.type, ui.interval, ui.record, ui.back};
    }
    struct {
        lv_obj_t *type{}, *interval{}, *record{}, *back{};
    } ui;

  private:
    unsigned type_ = 0, interval_ = 0;
    lv_obj_t *state_{}, *space_{}, *type_text_{}, *interval_text_{}, *record_text_{}, *back_text_{}, *feedback_{};
    lv_obj_t *hero_{}, *status_dot_{}, *record_icon_{};
};
} // namespace page
