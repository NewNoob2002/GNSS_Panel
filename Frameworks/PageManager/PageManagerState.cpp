/*
 * MIT License
 * Copyright (c) 2021 - 2024 _VIFEXTech
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
#include "PageLog.h"
#include "PageManager.h"

void PageManager::stateNext(PageBase* base)
{
    if (!base) {
        return;
    }

    sendEvent(EVENT_TYPE::PAGE_STATE_CHANGED, (void*)&base->_context.state);

    switch (base->_context.state) {
    case PageBase::STATE::IDLE:
        PAGE_LOG_INFO("Page(%s) state idle", base->_name);
        break;

    case PageBase::STATE::LOAD:
        base->_context.state = stateLoad(base);
        stateNext(base);
        break;

    case PageBase::STATE::WILL_APPEAR:
        base->_context.state = stateWillAppear(base);
        break;

    case PageBase::STATE::DID_APPEAR:
        base->_context.state = stateDidAppear(base);
        PAGE_LOG_INFO("Page(%s) state active", base->_name);
        break;

    case PageBase::STATE::ACTIVITY:
        PAGE_LOG_INFO("Page(%s) state active break", base->_name);
        base->_context.state = PageBase::STATE::WILL_DISAPPEAR;
        stateNext(base);
        break;

    case PageBase::STATE::WILL_DISAPPEAR:
        base->_context.state = stateWillDisappear(base);
        break;

    case PageBase::STATE::DID_DISAPPEAR:
        base->_context.state = stateDidDisappear(base);
        if (base->_context.state == PageBase::STATE::UNLOAD) {
            stateNext(base);
        }
        break;

    case PageBase::STATE::UNLOAD:
        base->_context.state = stateUnload(base);
        break;

    default:
        PAGE_LOG_ERROR("Page(%s) state[%d] was NOT FOUND!", base->_name, static_cast<int>(base->_context.state));
        break;
    }
}

PageBase::STATE PageManager::stateLoad(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state load", base->_name);

    if (base->_root != nullptr) {
        PAGE_LOG_ERROR("Page(%s) root must be nullptr", base->_name);
    }

    lv_obj_t* parent = base->_manager->getRootParent();
    lv_obj_t* root_obj = lv_obj_create(parent);

    lv_obj_set_scrollable(root_obj, false);
    lv_obj_set_user_data(root_obj, base);

    if (base->_context.backGestureDir != LV_DIR_NONE) {
        lv_obj_set_gesture_bubble(root_obj, false);
        lv_obj_add_event_cb(
            root_obj,
            [](lv_event_t* e) {
                auto self = (PageBase*)lv_event_get_user_data(e);
                auto dir = lv_indev_get_gesture_dir(lv_indev_active());
                if (dir == self->_context.backGestureDir) {
                    self->getManager()->pop();
                }
            },
            LV_EVENT_GESTURE,
            base);
    }

    if (_rootDefaultStyle) {
        lv_obj_add_style(root_obj, _rootDefaultStyle, LV_PART_MAIN);
    }

    base->_root = root_obj;
    lv_obj_add_event_cb(root_obj, onPageRootDelete, LV_EVENT_DELETE, base);
    base->onViewLoad();

    base->onViewDidLoad();

    if (base->_context.isDisableAutoCache) {
        PAGE_LOG_INFO("Page(%s) disable auto cache, reqEnableCache = %d", base->_name, base->_context.reqEnableCache);
        base->_context.isCached = base->_context.reqEnableCache;
    } else {
        PAGE_LOG_INFO("Page(%s) AUTO cached", base->_name);
        base->_context.isCached = true;
    }

    return PageBase::STATE::WILL_APPEAR;
}

PageBase::STATE PageManager::stateWillAppear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state will appear", base->_name);
    // 缓存页必须先可见，willAppear 中对根对象的聚焦才会生效。
    lv_obj_set_hidden(base->_root, false);
    base->onViewWillAppear();
    return PageBase::STATE::DID_APPEAR;
}

PageBase::STATE PageManager::stateDidAppear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state did appear", base->_name);
    base->onViewDidAppear();
    return PageBase::STATE::ACTIVITY;
}

PageBase::STATE PageManager::stateWillDisappear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state will disappear", base->_name);
    base->onViewWillDisappear();
    return PageBase::STATE::DID_DISAPPEAR;
}

PageBase::STATE PageManager::stateDidDisappear(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state did disappear", base->_name);
    lv_obj_set_hidden(base->_root, true);
    base->onViewDidDisappear();
    if (base->_context.isCached) {
        PAGE_LOG_INFO("Page(%s) has cached", base->_name);
        return PageBase::STATE::WILL_APPEAR;
    } else {
        return PageBase::STATE::UNLOAD;
    }
}

void
PageManager::onPageRootDelete(lv_event_t* e) {
    auto* base = static_cast<PageBase*>(lv_event_get_user_data(e));
    auto* manager = base->_manager;
    // 外部删除根时先停止借用控件的回调；正常卸载会提前移除此通知。
    lv_anim_delete(base->_root, nullptr);
    base->onViewUnload();
    base->_root = nullptr;
    lv_free(base->_context.param.ptr);
    base->_context.param = {};
    base->_context.isCached = false;
    base->_context.anim.isBusy = false;
    base->_context.state = PageBase::STATE::IDLE;
    manager->scrimDelete();
    if (manager->isSwitching()) {
        manager->switchReqCheck();
    }
    base->onViewDidUnload();
}

PageBase::STATE PageManager::stateUnload(PageBase* base)
{
    PAGE_LOG_INFO("Page(%s) state unload", base->_name);
    if (!base->_root) {
        PAGE_LOG_WARN("Page is unloaded!");
        return PageBase::STATE::IDLE;
    }

    lv_anim_delete(base->_root, nullptr);
    base->_context.anim.isBusy = false;
    base->onViewUnload();
    if (base->_context.param.ptr != nullptr) {
        PAGE_LOG_INFO("Page(%s) free param(%p)[%lu]", base->_name, base->_context.param.ptr,
                      static_cast<unsigned long>(base->_context.param.size));
        lv_free(base->_context.param.ptr);
        base->_context.param.ptr = nullptr;
        base->_context.param.size = 0;
    }

    /* Prevent another timer from modifying the style during async deletion */
    lv_obj_clean(base->_root);
    lv_obj_remove_style_all(base->_root);

    /* Delete after the end of the root animation life cycle */
    // 根对象延迟删除，先解除指向页面实例的事件；保留 App 的删除时置空通知。
    lv_obj_remove_event_cb_with_user_data(base->_root, nullptr, base);
    lv_obj_set_clickable(base->_root, false);
    lv_obj_set_user_data(base->_root, nullptr);
    lv_obj_delete_async(base->_root);
    base->_root = nullptr;
    base->_context.isCached = false;
    base->onViewDidUnload();
    return PageBase::STATE::IDLE;
}
