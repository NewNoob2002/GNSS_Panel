#pragma once

#include "DataProc/DataProc.h"
#include "lvgl.h"

namespace page {
/** @brief 应用线程的页面 Model；subject 只指向自有值，绝不保存 DataNode 回调参数。 */
template <class State> class Model : public DataNode {
  public:
    explicit Model(const char* name) : DataNode(name, DataProc::Broker()) {
        subject_ = lv_subject_create(LV_SUBJECT_TYPE_POINTER);
        if (subject_)
            lv_subject_set_pointer(subject_, &state_);
    }
    ~Model() override {
        lv_subject_delete(subject_);
    }
    const State& state() const { return state_; }
    lv_subject_t* subject() const { return subject_; }

  protected:
    State state_{};
    bool ready() const { return isRegistered() && subject_; }
    void changed() { lv_subject_notify(subject_); }

  private:
    lv_subject_t* subject_ = nullptr;
};
} // namespace page
