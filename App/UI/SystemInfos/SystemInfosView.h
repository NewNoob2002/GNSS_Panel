#pragma once

#include <array>
#include "SystemInfosModel.h"
#include "UI/Resource/ResourcePool.h"
#include "I18n.h"
#include "lvgl.h"

namespace page {
class SystemInfosView {
  public:
    SystemInfosView() = default;
    ~SystemInfosView() = default;

    void create(lv_obj_t* root);

    /** 页面出现后注册焦点；必须在前一页面清空焦点组之后调用。 */
    void group_init();

    void destroy();

    bool bind(lv_subject_t* subject);
    void unbind();

    /** Localized card titles and field names. */
    void apply_language() const;
    /** Card values from domain states. */
    void update(const SystemInfosState& state) const;

    /** Inventory controls in visible top-to-bottom order, after create(). */
    std::array<lv_obj_t*, 6> controls() const;

    typedef struct {
        lv_obj_t* cont;
        lv_obj_t* icon;
        lv_obj_t* label_name;
        lv_obj_t* label_info;
        lv_obj_t* label_data;
    } item_t;

    struct {
        item_t work;
        item_t gps;
        item_t wifi;
        item_t battery;
        item_t storage;
        item_t system;
    } ui;

    static void set_scroll_to_y(lv_obj_t* obj, int32_t y, lv_anim_enable_t en);
    static void on_focus(lv_group_t* e);

  private:
    struct {
        lv_style_t icon;
        lv_style_t focus;
        lv_style_t info;
        lv_style_t data;
    } style_;

    static void on_state(lv_observer_t* observer, lv_subject_t* subject);
    lv_observer_t* observer_ = nullptr;

    lv_obj_t* root_ = nullptr;
    void style_init();
    void style_reset();
    void item_create(item_t* item, lv_obj_t* par, const char* name, const char* img_src, const char* infos);
};
} // namespace page
