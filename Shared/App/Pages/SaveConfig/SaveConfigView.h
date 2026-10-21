#ifndef SAVECONFIG_VIEW_H
#define SAVECONFIG_VIEW_H

#include "Common/DataProc/DataProc.h"
#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
#include "lvgl.h"

namespace page {
class SaveConfigView {
  public:
    /** Create the power progress ring and text column below the visible status bar. */
    void create(lv_obj_t* root);
    /** Cancel animations borrowing widgets before the page is hidden or destroyed. */
    void stop() const;
    /** Reset the countdown and reveal the ring and text using translation only. */
    void appear();
    /** Refresh the action, cause and cancellation hint from the latest System state. */
    void update(const DataProc::SystemState& state);
    /** Return the unchanged eight-second progress animation for controller scheduling. */
    lv_anim_t* progress_animation() {
        return &progress_animation_;
    }

  private:
    /** Apply fine-grained progress and the ceiling of remaining whole seconds. */
    static void set_progress(void* obj, int32_t progress);
    /** Rotate the fixed shutdown segment without changing its value or children. */
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

#endif // SAVECONFIG_VIEW_H
