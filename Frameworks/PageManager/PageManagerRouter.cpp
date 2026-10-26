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
#include <string.h>

PageManager::RES_TYPE PageManager::replace(const char* name, const PageBase::PARAM* param)
{
    if (!name) {
        return RES_TYPE::ERR_PARAM;
    }
    /* Precheck event hook */
    auto res = sendEvent(EVENT_TYPE::PAGE_REPLACE, (void*)name);
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Check whether the animation of switching pages is being executed */
    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Check whether the stack is repeatedly pushed  */
    if (findPageInStack(name) != nullptr) {
        PAGE_LOG_ERROR("Page(%s) was multi push", name);
        return RES_TYPE::ERR_DUPLICATE;
    }

    /* Check if the page is registered in the page pool */
    PageBase* base = findPageInPool(name);

    if (!base) {
        PAGE_LOG_ERROR("Page(%s) was not install", name);
        return RES_TYPE::ERR_NOT_FOUND;
    }

    /* Get the top page of the stack */
    PageBase* top = getStackTop();

    if (!top) {
        PAGE_LOG_ERROR("Stack top is NULL");
        return RES_TYPE::ERR_OPERATION;
    }

    res = prepareParam(base, param);
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Force disable cache */
    top->_context.isCached = false;

    /* Synchronous automatic cache configuration */
    base->_context.isDisableAutoCache = base->_context.reqDisableAutoCache;
    base->_context.anim.navType = PAGE_ANIM::GLOBAL;

    /* Remove current page */
    _pageStack.pop();

    /* Push into the stack */
    _pageStack.push(base);

    PAGE_LOG_INFO("Page(%s) replace Page(%s) (param = %p)", name, top->_name, param);

    /* Page switching execution */
    return switchTo(base, true);
}

PageManager::RES_TYPE PageManager::push(const char* name, const PageBase::PARAM* param)
{
    return pushWithAnim(name, PAGE_ANIM::GLOBAL, param);
}

PageManager::RES_TYPE PageManager::pushWithAnim(const char* name, PAGE_ANIM anim, const PageBase::PARAM* param)
{
    if (!name) {
        return RES_TYPE::ERR_PARAM;
    }
    /* Precheck event hook */
    auto res = sendEvent(EVENT_TYPE::PAGE_PUSH, (void*)name);
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Check whether the animation of switching pages is being executed */
    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Check whether the stack is repeatedly pushed  */
    if (findPageInStack(name) != nullptr) {
        PAGE_LOG_ERROR("Page(%s) was multi push", name);
        return RES_TYPE::ERR_DUPLICATE;
    }

    /* Check if the page is registered in the page pool */
    PageBase* base = findPageInPool(name);

    if (!base) {
        PAGE_LOG_ERROR("Page(%s) was not install", name);
        return RES_TYPE::ERR_NOT_FOUND;
    }

    res = prepareParam(base, param);
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Synchronous automatic cache configuration */
    base->_context.isDisableAutoCache = base->_context.reqDisableAutoCache;

    /* The chosen transition belongs to this stack entry: it is reused when the page is left again */
    base->_context.anim.navType = anim;

    /* Push into the stack */
    _pageStack.push(base);

    PAGE_LOG_INFO("Page(%s) push >> [Screen] (param = %p)", name, param);

    /* Page switching execution */
    return switchTo(base, true);
}

PageManager::RES_TYPE PageManager::pop()
{
    /* Precheck event hook */
    auto res = sendEvent(EVENT_TYPE::PAGE_POP);
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Check whether the animation of switching pages is being executed */
    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) {
        return res;
    }

    if (_pageStack.size() <= 1) {
        PAGE_LOG_WARN("Bottom of stack, cat't pop");
        return RES_TYPE::ERR_OPERATION;
    }

    /* Get the top page of the stack */
    PageBase* top = getStackTop();
    LV_ASSERT_NULL(top);
    if (!top) {
        PAGE_LOG_ERROR("Stack top is NULL");
        return RES_TYPE::ERR_OPERATION;
    }

    /* Whether to turn off automatic cache */
    if (!top->_context.isDisableAutoCache) {
        PAGE_LOG_INFO("Page(%s) has auto cache, cache disabled", top->_name);
        top->_context.isCached = false;
    }

    PAGE_LOG_INFO("Page(%s) pop << [Screen]", top->_name);

    /* Page popup */
    _pageStack.pop();

    /* Get the next page */
    top = getStackTop();

    /* Page switching execution */
    return switchTo(top, false);
}

/** @brief Copy parameters atomically, preserving the old value on allocation failure. */
PageManager::RES_TYPE
PageManager::prepareParam(PageBase* base, const PageBase::PARAM* param) {
    if (!param) {
        return RES_TYPE::OK;
    }
    if (!param->ptr || !param->size) {
        return RES_TYPE::ERR_PARAM;
    }
    auto& saved = base->_context.param;
    void* buffer = saved.ptr;
    if (!buffer || saved.size != param->size) {
        buffer = lv_malloc(param->size);
        if (!buffer) {
            return RES_TYPE::ERR_OPERATION;
        }
    }
    lv_memmove(buffer, param->ptr, param->size);
    if (buffer != saved.ptr) {
        lv_free(saved.ptr);
    }
    saved.ptr = buffer;
    saved.size = param->size;
    return RES_TYPE::OK;
}

PageManager::RES_TYPE
PageManager::switchTo(PageBase* newNode, bool isEnterAct) {
    if (!newNode) {
        PAGE_LOG_ERROR("newNode is nullptr");
        return RES_TYPE::ERR_PARAM;
    }

    /* Whether page switching has been requested */
    if (_animState.isSwitchReq) {
        PAGE_LOG_WARN("Page switch busy, reqire(%s) is ignore", newNode->_name);
        return RES_TYPE::ERR_BUSY;
    }

    _animState.isSwitchReq = true;

    /* Record current page */
    _pageCurrent = newNode;

    /* If the current page has a cache */
    if (_pageCurrent->_context.isCached) {
        /* Direct display, no need to load */
        PAGE_LOG_INFO("Page(%s) has cached, appear driectly", _pageCurrent->_name);
        _pageCurrent->_context.state = PageBase::STATE::WILL_APPEAR;
    } else {
        /* Load page */
        _pageCurrent->_context.state = PageBase::STATE::LOAD;
    }

    if (_pagePrev != nullptr) {
        _pagePrev->_context.anim.isEnter = false;
    }

    _pageCurrent->_context.anim.isEnter = true;

    _animState.isEntering = isEnterAct;

    if (_animState.isEntering) {
        /* Update the animation configuration according to the current page */
        switchAnimTypeUpdate(_pageCurrent);
    }

    // 先建页再刷新动画时基，创建耗时不计入切换；此入口不能从动画回调重入。
    if (!_pageCurrent->_context.isCached) {
        _pageCurrent->_context.state = stateLoad(_pageCurrent);
    }
    _pageCurrent->_context.state = PageBase::STATE::WILL_APPEAR;
    lv_anim_refr_now();

    /* Update the state machine of the previous page */
    stateNext(_pagePrev);

    /* Update the state machine of the current page */
    stateNext(_pageCurrent);

    /* Since page creation takes a certain amount of time,
     * the animation is created uniformly after the state machine is executed,
     * and the animation start and end times are aligned.
     */
    switchAnimCreate(_pagePrev);
    switchAnimCreate(_pageCurrent);

    /* Move the layer, move the new page to the front */
    if (_animState.isEntering) {
        PAGE_LOG_INFO("Page ENTER is detect, move Page(%s) to foreground", _pageCurrent->_name);
        if (_pagePrev)
            lv_obj_move_to_index(_pagePrev->_root, -1);
        lv_obj_move_to_index(_pageCurrent->_root, -1);
    } else {
        PAGE_LOG_INFO("Page EXIT is detect, move Page(%s) to foreground", getPagePrevName());
        lv_obj_move_to_index(_pageCurrent->_root, -1);
        if (_pagePrev)
            lv_obj_move_to_index(_pagePrev->_root, -1);
    }

    /* After the page order is final: the scrim sits right beneath the covering page */
    scrimCreate();
    return RES_TYPE::OK;
}

PageManager::RES_TYPE
PageManager::forceUnload(PageBase* base) {
    if (!base) {
        PAGE_LOG_ERROR("Page is nullptr, Unload failed");
        return RES_TYPE::ERR_PARAM;
    }

    PAGE_LOG_INFO("Page(%s) Fource unloading...", base->_name);

    if (base->_context.state == PageBase::STATE::ACTIVITY) {
        PAGE_LOG_INFO("Page state is ACTIVITY, Disappearing...");
        base->onViewWillDisappear();
        base->onViewDidDisappear();
    }

    base->_context.state = stateUnload(base);

    return RES_TYPE::OK;
}

/** @brief Replace navigation history with an installed root after transitions finish. */
PageManager::RES_TYPE
PageManager::resetRoot(const char* name) {
    if (isSwitching()) {
        return RES_TYPE::ERR_BUSY;
    }
    if (!name || !findPageInPool(name)) {
        return RES_TYPE::ERR_NOT_FOUND;
    }
    // 在清栈前执行事件钩子，拒绝导航时保留当前页面与历史。
    auto res = sendEvent(EVENT_TYPE::PAGE_PUSH, const_cast<char*>(name));
    if (res != RES_TYPE::OK) {
        return res;
    }
    clearStack();
    _pageCurrent = _pagePrev = nullptr;
    PageBase* base = findPageInPool(name);
    base->_context.isDisableAutoCache = base->_context.reqDisableAutoCache;
    base->_context.anim.navType = PAGE_ANIM::GLOBAL;
    _pageStack.push(base);
    return switchTo(base, true);
}

/** @brief Load one idle cacheable page and keep its root hidden until navigation. */
PageManager::RES_TYPE
PageManager::preload(const char* name) {
    if (isSwitching()) {
        return RES_TYPE::ERR_BUSY;
    }
    PageBase* base = findPageInPool(name);
    if (!base) {
        return RES_TYPE::ERR_NOT_FOUND;
    }
    if (base->_root || base->_context.state != PageBase::STATE::IDLE) {
        return RES_TYPE::ERR_OPERATION;
    }
    base->_context.isDisableAutoCache = base->_context.reqDisableAutoCache;
    base->_context.state = stateLoad(base);
    lv_obj_set_hidden(base->_root, true);
    if (!base->_context.isCached) {
        base->_context.state = stateUnload(base);
        return RES_TYPE::ERR_OPERATION;
    }
    return RES_TYPE::OK;
}

PageManager::RES_TYPE PageManager::backHome()
{
    /* Precheck event hook */
    auto res = sendEvent(EVENT_TYPE::PAGE_BACK_HOME);
    if (res != RES_TYPE::OK) {
        return res;
    }

    /* Check whether the animation of switching pages is being executed */
    res = switchAnimStateCheck();
    if (res != RES_TYPE::OK) {
        return res;
    }

    if (_pageStack.empty()) {
        return RES_TYPE::ERR_OPERATION;
    }
    clearStack(true);

    _pagePrev = nullptr;

    PageBase* home = getStackTop();

    return switchTo(home, false);
}

PageManager::RES_TYPE PageManager::switchAnimStateCheck()
{
    if (_animState.isSwitchReq || _animState.isBusy) {
        PAGE_LOG_WARN(
            "Page switch busy[AnimState.isSwitchReq = %d,"
            "AnimState.isBusy = %d],"
            "request ignored",
            _animState.isSwitchReq,
            _animState.isBusy);
        return RES_TYPE::ERR_BUSY;
    }

    return RES_TYPE::OK;
}

PageManager::RES_TYPE PageManager::switchReqCheck()
{
    auto res = RES_TYPE::ERR_BUSY;
    bool lastNodeBusy = _pagePrev && _pagePrev->_context.anim.isBusy;

    if (!_pageCurrent->_context.anim.isBusy && !lastNodeBusy) {
        PAGE_LOG_INFO("----Page switch was all finished----");
        _animState.isSwitchReq = false;
        res = RES_TYPE::OK;
        _pagePrev = _pageCurrent;
    } else {
        if (_pageCurrent->_context.anim.isBusy) {
            PAGE_LOG_WARN("Page PageCurrent(%s) is busy", _pageCurrent->_name);
        } else {
            PAGE_LOG_WARN("Page PagePrev(%s) is busy", getPagePrevName());
        }
    }

    return res;
}

void PageManager::onSwitchAnimFinish(lv_anim_t* a)
{
    auto base = (PageBase*)lv_anim_get_user_data(a);
    auto manager = base->_manager;

    PAGE_LOG_INFO("Page(%s) anim finish", base->_name);

    manager->stateNext(base);
    base->_context.anim.isBusy = false;
    auto res = manager->switchReqCheck();

    if (!manager->_animState.isEntering && res == RES_TYPE::OK) {
        manager->switchAnimTypeUpdate(manager->_pageCurrent);
    }
}

void PageManager::switchAnimCreate(PageBase* base)
{
    if (!base) {
        return;
    }

    LOAD_ANIM_ATTR animAttr;
    if (getCurrentLoadAnimAttr(&animAttr) != RES_TYPE::OK) {
        return;
    }

    // 缓存页可能带着另一轴的退出偏移，进入时只保留本次动画驱动的属性。
    if (base->_context.anim.isEnter) {
        const auto type = getCurrentLoadAnimType();
        const bool horizontal = isHorizontalLoadAnim(type);
        const bool vertical = isVerticalLoadAnim(type);
        if (!horizontal) {
            lv_obj_set_x(base->_root, 0);
        }
        if (!vertical) {
            lv_obj_set_y(base->_root, 0);
        }
        if (type != PAGE_ANIM::FADE_ON) {
            lv_obj_set_style_opa(base->_root, LV_OPA_COVER, LV_PART_MAIN);
        }
    }

    lv_anim_t a;
    animDefaultInit(&a);
    lv_anim_set_user_data(&a, base);
    lv_anim_set_var(&a, base->_root);
    lv_anim_set_completed_cb(&a, onSwitchAnimFinish);
    lv_anim_set_exec_cb(&a, animAttr.setter);

    int32_t start = 0;

    if (animAttr.getter) {
        start = animAttr.getter(base->_root);
    }

    if (_animState.isEntering) {
        if (base->_context.anim.isEnter) {
            lv_anim_set_values(
                &a,
                animAttr.push.enter.start,
                animAttr.push.enter.end);
        } else /* Exit */
        {
            lv_anim_set_values(
                &a,
                start,
                animAttr.push.exit.end);
        }
    } else /* Pop */
    {
        if (base->_context.anim.isEnter) {
            lv_anim_set_values(
                &a,
                animAttr.pop.enter.start,
                animAttr.pop.enter.end);
        } else /* Exit */
        {
            lv_anim_set_values(
                &a,
                start,
                animAttr.pop.exit.end);
        }
    }

    lv_anim_start(&a);
    base->_context.anim.isBusy = true;
}

void PageManager::setGlobalLoadAnim(PAGE_ANIM anim, uint32_t time, lv_anim_path_cb_t path)
{
    _animState.global.type = anim;
    _animState.global.duration = time;
    _animState.global.path = path ? path : lv_anim_path_linear;

    PAGE_LOG_INFO("Set global load anim type = %d", static_cast<int>(anim));
}

void PageManager::switchAnimTypeUpdate(PageBase* base)
{
    if (base->_context.anim.attr.type == PAGE_ANIM::GLOBAL) {
        PAGE_LOG_INFO("Page(%s) anim.type was not set, use AnimState.global.type = %d", base->_name,
                      static_cast<int>(_animState.global.type));
        _animState.current = _animState.global;
    } else {
        PAGE_LOG_INFO("Page(%s) custom anim.type set = %d", base->_name,
                      static_cast<int>(base->_context.anim.attr.type));
        _animState.current = base->_context.anim.attr;
    }
    if (base->_context.anim.navType != PAGE_ANIM::GLOBAL) {
        PAGE_LOG_INFO("Page(%s) navigation anim.type = %d", base->_name, static_cast<int>(base->_context.anim.navType));
        _animState.current.type = base->_context.anim.navType;
    }
}

void PageManager::animDefaultInit(lv_anim_t* a)
{
    lv_anim_init(a);

    uint32_t time = (getCurrentLoadAnimType() == PAGE_ANIM::NONE) ? 0 : _animState.current.duration;
    lv_anim_set_duration(a, time);
    lv_anim_set_path_cb(a, _animState.current.path);
}
