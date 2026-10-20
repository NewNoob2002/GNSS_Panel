#ifndef SYSTEM_INFOS_VIEW_H
#define SYSTEM_INFOS_VIEW_H

#include "Common/DataProc/DataProc_Def.h"
#include "Resource/ResourcePool.h"
#include "Utils/I18n/I18n.h"
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

    /** Render localized inventory cards and the retained initialization error count. */
    void apply_language(const DataProc::SystemState& state = {}) const;

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

    static void set_scroll_to_y(lv_obj_t* obj, lv_coord_t y, lv_anim_enable_t en);

    static void on_focus(lv_group_t* e);

  private:
    struct {
        lv_style_t icon;
        lv_style_t focus;
        lv_style_t info;
        lv_style_t data;
    } style_;

  private:
    void style_init();

    void style_reset();

    void item_create(item_t* item, lv_obj_t* par, const char* name, const char* img_src, const char* infos);
};
} // namespace page

#endif // SYSTEM_INFOS_VIEW_H
