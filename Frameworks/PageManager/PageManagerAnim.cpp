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

/* PUSH_*: the covered page travels a third of the distance, so it trails the new page instead of being wiped. */
#define PAGE_PUSH_PARALLAX_DIV 3

/* Scrim animation runs on its own 0..SCRIM_PROGRESS_MAX scale with the page animations' duration and path. */
#define SCRIM_PROGRESS_MAX 1024

namespace {
bool
isHorizontalAnim(PAGE_ANIM anim) {
    return anim == PAGE_ANIM::OVER_LEFT || anim == PAGE_ANIM::OVER_RIGHT || anim == PAGE_ANIM::MOVE_LEFT
           || anim == PAGE_ANIM::MOVE_RIGHT || anim == PAGE_ANIM::PUSH_LEFT || anim == PAGE_ANIM::PUSH_RIGHT;
}

bool
isVerticalAnim(PAGE_ANIM anim) {
    return anim == PAGE_ANIM::OVER_TOP || anim == PAGE_ANIM::OVER_BOTTOM || anim == PAGE_ANIM::MOVE_TOP
           || anim == PAGE_ANIM::MOVE_BOTTOM || anim == PAGE_ANIM::PUSH_TOP || anim == PAGE_ANIM::PUSH_BOTTOM;
}

/** @brief One page stays (partly) underneath the other, so a scrim can dim it. */
bool
isCoveringAnim(PAGE_ANIM anim) {
    return anim == PAGE_ANIM::OVER_LEFT || anim == PAGE_ANIM::OVER_RIGHT || anim == PAGE_ANIM::OVER_TOP
           || anim == PAGE_ANIM::OVER_BOTTOM || anim == PAGE_ANIM::PUSH_LEFT || anim == PAGE_ANIM::PUSH_RIGHT
           || anim == PAGE_ANIM::PUSH_TOP || anim == PAGE_ANIM::PUSH_BOTTOM;
}
} // namespace

PageManager::RES_TYPE PageManager::getLoadAnimAttr(PAGE_ANIM anim, LOAD_ANIM_ATTR* attr)
{
    lv_obj_t* rootParent = getRootParent();
    lv_obj_update_layout(rootParent);
    int32_t hor = lv_obj_get_width(rootParent);
    int32_t ver = lv_obj_get_height(rootParent);

    switch (anim) {
    case PAGE_ANIM::OVER_LEFT:
        attr->push.enter.start = hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = hor;
        break;

    case PAGE_ANIM::OVER_RIGHT:
        attr->push.enter.start = -hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -hor;
        break;

    case PAGE_ANIM::OVER_TOP:
        attr->push.enter.start = ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = ver;
        break;

    case PAGE_ANIM::OVER_BOTTOM:
        attr->push.enter.start = -ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = 0;

        attr->pop.enter.start = 0;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -ver;
        break;

    case PAGE_ANIM::MOVE_LEFT:
        attr->push.enter.start = hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = -hor;

        attr->pop.enter.start = -hor;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = hor;
        break;

    case PAGE_ANIM::MOVE_RIGHT:
        attr->push.enter.start = -hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = hor;

        attr->pop.enter.start = hor;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -hor;
        break;

    case PAGE_ANIM::MOVE_TOP:
        attr->push.enter.start = ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = -ver;

        attr->pop.enter.start = -ver;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = ver;
        break;

    case PAGE_ANIM::MOVE_BOTTOM:
        attr->push.enter.start = -ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = ver;

        attr->pop.enter.start = ver;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -ver;
        break;

    case PAGE_ANIM::PUSH_LEFT:
        attr->push.enter.start = hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = -hor / PAGE_PUSH_PARALLAX_DIV;

        attr->pop.enter.start = -hor / PAGE_PUSH_PARALLAX_DIV;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = hor;
        break;

    case PAGE_ANIM::PUSH_RIGHT:
        attr->push.enter.start = -hor;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = hor / PAGE_PUSH_PARALLAX_DIV;

        attr->pop.enter.start = hor / PAGE_PUSH_PARALLAX_DIV;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -hor;
        break;

    case PAGE_ANIM::PUSH_TOP:
        attr->push.enter.start = ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = -ver / PAGE_PUSH_PARALLAX_DIV;

        attr->pop.enter.start = -ver / PAGE_PUSH_PARALLAX_DIV;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = ver;
        break;

    case PAGE_ANIM::PUSH_BOTTOM:
        attr->push.enter.start = -ver;
        attr->push.enter.end = 0;
        attr->push.exit.start = 0;
        attr->push.exit.end = ver / PAGE_PUSH_PARALLAX_DIV;

        attr->pop.enter.start = ver / PAGE_PUSH_PARALLAX_DIV;
        attr->pop.enter.end = 0;
        attr->pop.exit.start = 0;
        attr->pop.exit.end = -ver;
        break;

    case PAGE_ANIM::FADE_ON:
        attr->push.enter.start = LV_OPA_TRANSP;
        attr->push.enter.end = LV_OPA_COVER;
        attr->push.exit.start = LV_OPA_COVER;
        attr->push.exit.end = LV_OPA_COVER;

        attr->pop.enter.start = LV_OPA_COVER;
        attr->pop.enter.end = LV_OPA_COVER;
        attr->pop.exit.start = LV_OPA_COVER;
        attr->pop.exit.end = LV_OPA_TRANSP;
        break;

    case PAGE_ANIM::FADE_THROUGH:
        /* 页面根不动，仍保留动画完成回调推进状态机。 */
        attr->setter = [](void*, int32_t) {};
        break;

    case PAGE_ANIM::NONE:
        /* Use default NONE */
        break;

    default: PAGE_LOG_ERROR("Load anim type error: %d", static_cast<int>(anim)); return RES_TYPE::ERR_PARAM;
    }

    /* Determine the setter and getter of the animation */
    if (isHorizontalAnim(anim)) {
        attr->setter = [](void* obj, int32_t v) {
            lv_obj_set_x((lv_obj_t*)obj, v);
        };
        attr->getter = [](void* obj) { return (int32_t)lv_obj_get_style_x((lv_obj_t*)obj, LV_PART_MAIN); };
        return RES_TYPE::OK;
    }
    if (isVerticalAnim(anim)) {
        attr->setter = [](void* obj, int32_t v) {
            lv_obj_set_y((lv_obj_t*)obj, v);
        };
        attr->getter = [](void* obj) { return (int32_t)lv_obj_get_style_y((lv_obj_t*)obj, LV_PART_MAIN); };
        return RES_TYPE::OK;
    }
    switch (anim) {
    case PAGE_ANIM::FADE_ON:
        attr->setter = [](void* obj, int32_t v) {
            lv_obj_set_style_opa((lv_obj_t*)obj, (lv_opa_t)v, LV_PART_MAIN);
        };
        attr->getter = [](void* obj) {
            return (int32_t)lv_obj_get_style_opa((lv_obj_t*)obj, LV_PART_MAIN);
        };
        break;

    default:
        break;
    }

    return RES_TYPE::OK;
}

bool PageManager::isHorizontalLoadAnim(PAGE_ANIM anim)
{
    return isHorizontalAnim(anim);
}

bool PageManager::isVerticalLoadAnim(PAGE_ANIM anim)
{
    return isVerticalAnim(anim);
}

void PageManager::scrimCreate()
{
    scrimDelete();
    const PAGE_ANIM type = getCurrentLoadAnimType();
    const bool fadeThrough = type == PAGE_ANIM::FADE_THROUGH;
    if ((!fadeThrough && (_scrimOpa == LV_OPA_TRANSP || !isCoveringAnim(type))) || !_pagePrev
        || _pagePrev == _pageCurrent) {
        return;
    }

    /* Push: the new page covers the previous one; pop: the leaving page uncovers the revealed one. */
    PageBase* covering = _animState.isEntering ? _pageCurrent : _pagePrev;
    if (!covering->_root || !_pageCurrent->_root || !_pagePrev->_root) {
        return;
    }

    LOAD_ANIM_ATTR attr;
    if (getCurrentLoadAnimAttr(&attr) != RES_TYPE::OK) {
        return;
    }

    _scrim.fadeThrough = fadeThrough;
    _scrim.horizontal = isHorizontalAnim(type);
    _scrim.darken = _animState.isEntering;
    if (!fadeThrough) {
        lv_obj_update_layout(covering->_root);
        _scrim.length = _scrim.horizontal ? lv_obj_get_width(covering->_root) : lv_obj_get_height(covering->_root);
        // 遮罩自行按同一进度计算覆盖页位置：LVGL 新动画插在链表头部、先于页面动画执行，读取实际坐标会落后一帧。
        if (_animState.isEntering) {
            _scrim.start = attr.push.enter.start;
            _scrim.end = attr.push.enter.end;
        } else {
            _scrim.start = attr.getter(covering->_root);
            _scrim.end = attr.pop.exit.end;
        }
    }

    lv_obj_t* scrim = lv_obj_create(getRootParent());
    lv_obj_remove_style_all(scrim);
    lv_obj_set_clickable(scrim, false);
    lv_obj_set_scrollable(scrim, false);
    lv_obj_set_ignore_layout(scrim, true);
    lv_obj_set_style_bg_color(scrim, fadeThrough ? _fadeThroughColor : lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(scrim, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_add_event_cb(scrim, onScrimDelete, LV_EVENT_DELETE, this);
    /* 纯色过渡盖住离开页；位移过渡仍放在覆盖页下方。 */
    if (!fadeThrough) {
        lv_obj_move_to_index(scrim, lv_obj_get_index(covering->_root));
    }
    _scrim.obj = scrim;
    scrimUpdate(0);

    lv_anim_t a;
    animDefaultInit(&a);
    lv_anim_set_var(&a, scrim);
    lv_anim_set_user_data(&a, this);
    lv_anim_set_values(&a, 0, SCRIM_PROGRESS_MAX);
    lv_anim_set_custom_exec_cb(&a, onScrimAnim);
    lv_anim_set_completed_cb(&a, onScrimFinish);
    if (!lv_anim_start(&a)) {
        scrimDelete();
    } else if (fadeThrough) {
        lv_obj_set_hidden(_pageCurrent->_root, true);
    }
}

void PageManager::scrimDelete()
{
    if (_scrim.fadeThrough) {
        if (_pageCurrent && _pageCurrent->_root) {
            lv_obj_set_hidden(_pageCurrent->_root, false);
        }
        _scrim.fadeThrough = false;
    }
    lv_obj_t* scrim = _scrim.obj;
    if (!scrim) {
        return;
    }
    _scrim.obj = nullptr;
    lv_obj_remove_event_cb_with_user_data(scrim, onScrimDelete, this);
    lv_obj_delete(scrim);
}

void PageManager::scrimUpdate(int32_t progress)
{
    lv_obj_t* scrim = _scrim.obj;
    if (!scrim) {
        return;
    }
    if (_scrim.fadeThrough) {
        lv_obj_set_pos(scrim, 0, 0);
        lv_obj_set_size(scrim, lv_pct(100), lv_pct(100));
        lv_obj_set_style_bg_opa(scrim, (lv_opa_t)(LV_OPA_COVER * progress / SCRIM_PROGRESS_MAX), LV_PART_MAIN);
        return;
    }
    lv_obj_t* parent = getRootParent();
    const int32_t span = _scrim.horizontal ? lv_obj_get_content_width(parent) : lv_obj_get_content_height(parent);
    const int32_t pos = _scrim.start + (_scrim.end - _scrim.start) * progress / SCRIM_PROGRESS_MAX;

    /* The covering page touches one edge; dim only the strip it leaves uncovered. */
    int32_t from = 0;
    int32_t to = 0;
    if (pos > 0) {
        to = LV_MIN(pos, span);
    } else {
        from = LV_MAX(pos + _scrim.length, 0);
        to = span;
    }

    if (to <= from) {
        lv_obj_set_hidden(scrim, true);
        return;
    }
    lv_obj_set_hidden(scrim, false);
    if (_scrim.horizontal) {
        lv_obj_set_pos(scrim, from, 0);
        lv_obj_set_size(scrim, to - from, lv_pct(100));
    } else {
        lv_obj_set_pos(scrim, 0, from);
        lv_obj_set_size(scrim, lv_pct(100), to - from);
    }
    const int32_t level = _scrim.darken ? progress : SCRIM_PROGRESS_MAX - progress;
    lv_obj_set_style_bg_opa(scrim, (lv_opa_t)(_scrimOpa * level / SCRIM_PROGRESS_MAX), LV_PART_MAIN);
}

void PageManager::onScrimAnim(lv_anim_t* a, int32_t progress)
{
    auto* manager = static_cast<PageManager*>(lv_anim_get_user_data(a));
    manager->scrimUpdate(progress);
}

void PageManager::onScrimFinish(lv_anim_t* a)
{
    auto* manager = static_cast<PageManager*>(lv_anim_get_user_data(a));
    /* Deleting the animated object from its own completion callback is supported by lv_anim */
    manager->scrimDelete();
}

void PageManager::onScrimDelete(lv_event_t* e)
{
    auto* manager = static_cast<PageManager*>(lv_event_get_user_data(e));
    manager->_scrim.obj = nullptr;
    manager->scrimDelete();
}
