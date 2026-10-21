/*
 * MIT License
 * Copyright (c) 2021 _VIFEXTech
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <cstdlib>
#include "PM_Log.h"
#include "PageManager.h"

#define CONSTRAIN(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

/* The distance threshold to trigger the drag */
#define PM_INDEV_DEF_DRAG_THROW   20

/**
  * @brief  Page drag event callback
  * @param  event: Pointer to event structure
  * @retval None
  */
void PageManager::on_root_drag_event(lv_event_t* event) {
    const lv_event_code_t event_code = lv_event_get_code(event);

    if (!(event_code == LV_EVENT_PRESSED || event_code == LV_EVENT_PRESSING || event_code == LV_EVENT_RELEASED)) {
        return;
    }

    const lv_indev_t* indev = lv_indev_get_act();
    if (indev == nullptr || lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) {
        return;
    }

    auto* root = lv_event_get_current_target(event);
    auto* base = static_cast<PageBase*>(lv_event_get_user_data(event));

    if (base == nullptr) {
        PM_LOG_ERROR("Page base is NULL");
        return;
    }

    PageManager* manager = base->page_manager;
    LoadAnimAttr_t anim_attr;

    if (!manager->get_current_load_anim_attr(&anim_attr)) {
        PM_LOG_ERROR("Can't get current anim attr");
        return;
    }

    if (event_code == LV_EVENT_PRESSED) {
        if (manager->anim_state_.is_switch_req) {
            return;
        }

        if (manager->anim_state_.is_busy) {
            PM_LOG_INFO("Root drag anim interrupted");
            lv_anim_del(root, anim_attr.setter);
            manager->anim_state_.is_busy = false;
        }

        /* Temporary showing the bottom page */
        const PageBase* bottom_page = manager->get_stack_top_after();
        if (bottom_page == nullptr || bottom_page->root == nullptr) {
            return;
        }

        lv_obj_clear_flag(bottom_page->root, LV_OBJ_FLAG_HIDDEN);
        manager->anim_state_.is_dragging = true;
    } else if (event_code == LV_EVENT_PRESSING) {
        if (!manager->anim_state_.is_dragging) {
            return;
        }

        lv_coord_t cur = anim_attr.getter(root);

        const lv_coord_t max = std::max(anim_attr.pop.exit.start, anim_attr.pop.exit.end);
        const lv_coord_t min = std::min(anim_attr.pop.exit.start, anim_attr.pop.exit.end);

        lv_point_t offset;
        lv_indev_get_vect(lv_indev_get_act(), &offset);

        if (anim_attr.drag_dir == ROOT_DRAG_DIR_HOR) {
            cur += offset.x;
        } else if (anim_attr.drag_dir == ROOT_DRAG_DIR_VER) {
            cur += offset.y;
        }

        anim_attr.setter(root, CONSTRAIN(cur, min, max));
    } else if (event_code == LV_EVENT_RELEASED) {
        if (manager->anim_state_.is_switch_req || !manager->anim_state_.is_dragging) {
            return;
        }
        manager->anim_state_.is_dragging = false;

        const lv_coord_t offset_sum = anim_attr.push.enter.end - anim_attr.push.enter.start;

        lv_coord_t x_predict = 0;
        lv_coord_t y_predict = 0;
        root_get_drag_predict(&x_predict, &y_predict);

        const lv_coord_t start = anim_attr.getter(root);
        lv_coord_t end = start;

        if (anim_attr.drag_dir == ROOT_DRAG_DIR_HOR) {
            end += x_predict;
            PM_LOG_INFO("Root drag x_predict = %d", end);
        } else if (anim_attr.drag_dir == ROOT_DRAG_DIR_VER) {
            end += y_predict;
            PM_LOG_INFO("Root drag y_predict = %d", end);
        }

        if (std::abs(end) > std::abs((int)offset_sum) / 2) {
            lv_async_call(on_root_async_leave, base);
        } else if (end != anim_attr.push.enter.end) {
            manager->anim_state_.is_busy = true;

            lv_anim_t a;
            manager->anim_default_init(&a);
            lv_anim_set_user_data(&a, manager);
            lv_anim_set_var(&a, root);
            lv_anim_set_values(&a, start, anim_attr.push.enter.end);
            lv_anim_set_exec_cb(&a, anim_attr.setter);
            lv_anim_set_ready_cb(&a, on_root_drag_anim_finish);
            lv_anim_start(&a);
            PM_LOG_INFO("Root drag anim start");
        }
    }
}

/**
  * @brief  Drag animation end event callback
  * @param  a: Pointer to animation
  * @retval None
  */
void PageManager::on_root_drag_anim_finish(lv_anim_t* a) {
    auto* manager = static_cast<PageManager*>(lv_anim_get_user_data(a));
    PM_LOG_INFO("Root drag anim finish");
    manager->anim_state_.is_busy = false;

    /* Hide the bottom page */
    if (const PageBase* bottom_page = manager->get_stack_top_after(); bottom_page) {
        lv_obj_add_flag(bottom_page->root, LV_OBJ_FLAG_HIDDEN);
    }
}

/**
  * @brief  Enable root's drag function
  * @param  root: Pointer to the root object
  * @retval None
  */
void PageManager::root_enable_drag(lv_obj_t* root) {
    auto* base = static_cast<PageBase*>(lv_obj_get_user_data(root));
    lv_obj_add_event_cb(root, on_root_drag_event, LV_EVENT_ALL, base);
    PM_LOG_INFO("Page(%s) Root drag enabled", base->page_name);
}

/**
  * @brief  Asynchronous callback when dragging ends
  * @param  data: Pointer to the base class of the page
  * @retval None
  */
void PageManager::on_root_async_leave(void* data) {
    auto* base = static_cast<PageBase*>(data);
    PM_LOG_INFO("Page(%s) send event: LV_EVENT_LEAVE, need to handle...", base->page_name);
    lv_event_send(base->root, LV_EVENT_LEAVE, base);
}

/**
  * @brief  Get drag inertia prediction stop point
  * @param  x: x stop point
  * @param  y: y stop point
  * @retval None
  */
void PageManager::root_get_drag_predict(lv_coord_t* x, lv_coord_t* y) {
    const lv_indev_t* indev = lv_indev_get_act();
    lv_point_t vect;
    lv_indev_get_vect(indev, &vect);

    lv_coord_t y_predict = 0;
    lv_coord_t x_predict = 0;

    while (vect.y != 0) {
        y_predict += vect.y;
        vect.y = vect.y * (100 - PM_INDEV_DEF_DRAG_THROW) / 100;
    }

    while (vect.x != 0) {
        x_predict += vect.x;
        vect.x = vect.x * (100 - PM_INDEV_DEF_DRAG_THROW) / 100;
    }

    *x = x_predict;
    *y = y_predict;
}
