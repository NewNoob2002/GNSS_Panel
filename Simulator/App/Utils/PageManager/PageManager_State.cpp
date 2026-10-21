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
#include "PM_Log.h"
#include "PageManager.h"

/**
  * @brief  Update page state machine
  * @param  base: Pointer to the updated page
  * @retval None
  */
void PageManager::state_update(PageBase* base) {
    if (base == nullptr) {
        return;
    }

    switch (base->priv.state) {
        case PageBase::PAGE_STATE_IDLE:
            PM_LOG_INFO("Page(%s) state idle", base->page_name);
            break;

        case PageBase::PAGE_STATE_LOAD:
            base->priv.state = state_load_execute(base);
            state_update(base);
            break;

        case PageBase::PAGE_STATE_WILL_APPEAR:
            base->priv.state = state_will_appear_execute(base);
            break;

        case PageBase::PAGE_STATE_DID_APPEAR:
            base->priv.state = state_did_appear_execute(base);
            PM_LOG_INFO("Page(%s) state active", base->page_name);
            break;

        case PageBase::PAGE_STATE_ACTIVITY:
            PM_LOG_INFO("Page(%s) state active break", base->page_name);
            base->priv.state = PageBase::PAGE_STATE_WILL_DISAPPEAR;
            state_update(base);
            break;

        case PageBase::PAGE_STATE_WILL_DISAPPEAR:
            base->priv.state = state_will_disappear_execute(base);
            break;

        case PageBase::PAGE_STATE_DID_DISAPPEAR:
            base->priv.state = state_did_disappear_execute(base);
            if (base->priv.state == PageBase::PAGE_STATE_UNLOAD) {
                state_update(base);
            }
            break;

        case PageBase::PAGE_STATE_UNLOAD:
            base->priv.state = state_unload_execute(base);
            break;

        default:
            PM_LOG_ERROR("Page(%s) state[%d] was NOT FOUND!", base->page_name, base->priv.state);
            break;
    }
}

/**
  * @brief  Page loading status
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
PageBase::State_t PageManager::state_load_execute(PageBase* base) {
    PM_LOG_INFO("Page(%s) state load", base->page_name);

    if (base->root != nullptr) {
        PM_LOG_ERROR("Page(%s) root must be nullptr", base->page_name);
    }

    lv_obj_t* root_obj = lv_obj_create(lv_scr_act());

    lv_obj_clear_flag(root_obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(root_obj, base);

    if (root_default_style_) {
        lv_obj_add_style(root_obj, root_default_style_, LV_PART_MAIN);
    }

    base->root = root_obj;
    base->on_view_load();

    if (get_is_over_anim(get_current_load_anim_type())) {
        if (const PageBase* bottom_page = get_stack_top_after();
            bottom_page != nullptr && bottom_page->priv.is_cached) {
            LoadAnimAttr_t anim_attr;
            if (get_current_load_anim_attr(&anim_attr)) {
                if (anim_attr.drag_dir != ROOT_DRAG_DIR_NONE) {
                    root_enable_drag(base->root);
                }
            }
        }
    }

    base->on_view_did_load();

    if (base->priv.is_disable_auto_cache) {
        PM_LOG_INFO("Page(%s) disable auto cache, ReqEnableCache = %d", base->page_name, base->priv.req_enable_cache);
        base->priv.is_cached = base->priv.req_enable_cache;
    } else {
        PM_LOG_INFO("Page(%s) AUTO cached", base->page_name);
        base->priv.is_cached = true;
    }

    return PageBase::PAGE_STATE_WILL_APPEAR;
}

/**
  * @brief  The page is about to show the status
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
PageBase::State_t PageManager::state_will_appear_execute(PageBase* base) const {
    PM_LOG_INFO("Page(%s) state will appear", base->page_name);
    // Show the root before the page sets up focus: a cached or preloaded root is still hidden here, and a page
    // that focuses its root (StarMap) would otherwise skip it. Nothing renders until this timer cycle ends.
    lv_obj_clear_flag(base->root, LV_OBJ_FLAG_HIDDEN);
    base->on_view_will_appear();
    switch_anim_create(base);
    return PageBase::PAGE_STATE_DID_APPEAR;
}

/**
  * @brief  The status of the page display
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
PageBase::State_t PageManager::state_did_appear_execute(PageBase* base) {
    PM_LOG_INFO("Page(%s) state did appear", base->page_name);
    base->on_view_did_appear();
    return PageBase::PAGE_STATE_ACTIVITY;
}

/**
  * @brief  The page is about to disappear
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
PageBase::State_t PageManager::state_will_disappear_execute(PageBase* base) const {
    PM_LOG_INFO("Page(%s) state will disappear", base->page_name);
    base->on_view_will_disappear();
    switch_anim_create(base);
    return PageBase::PAGE_STATE_DID_DISAPPEAR;
}

/**
  * @brief  Page disappeared end state
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
PageBase::State_t PageManager::state_did_disappear_execute(PageBase* base) {
    PM_LOG_INFO("Page(%s) state did disappear", base->page_name);
    lv_obj_add_flag(base->root, LV_OBJ_FLAG_HIDDEN);
    base->on_view_did_disappear();
    if (base->priv.is_cached) {
        PM_LOG_INFO("Page(%s) has cached", base->page_name);
        return PageBase::PAGE_STATE_WILL_APPEAR;
    } else {
        return PageBase::PAGE_STATE_UNLOAD;
    }
}

/**
  * @brief  Page unload complete
  * @param  base: Pointer to the updated page
  * @retval Next state
  */
PageBase::State_t PageManager::state_unload_execute(PageBase* base) {
    PM_LOG_INFO("Page(%s) state unload", base->page_name);
    if (base->root == nullptr) {
        PM_LOG_WARN("Page is loaded!");
        goto Exit;
    }

    base->on_view_unload();
    if (base->priv.stash.ptr != nullptr && base->priv.stash.size != 0) {
        PM_LOG_INFO("Page(%s) free stash(0x%p)[%lu]", base->page_name, base->priv.stash.ptr,
                    static_cast<unsigned long>(base->priv.stash.size));
        lv_mem_free(base->priv.stash.ptr);
        base->priv.stash.ptr = nullptr;
        base->priv.stash.size = 0;
    }

    /* state_unload_execute runs after the switch animation has finished. Delete
       synchronously so callbacks cannot outlive the owning PageBase instance. */
    lv_obj_del(base->root);
    base->root = nullptr;
    base->priv.is_cached = false;
    base->on_view_did_unload();

Exit:
    return PageBase::PAGE_STATE_IDLE;
}
