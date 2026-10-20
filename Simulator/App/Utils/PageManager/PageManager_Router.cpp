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
   * @brief  Enter a new page, replace the old page
   * @param  name: The name of the page to enter
   * @param  stash: Parameters passed to the new page
   * @retval Return true if successful
   */
bool PageManager::replace(const char* name, const PageBase::Stash_t* stash) {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return false;
    }

    /* Check whether the stack is repeatedly pushed  */
    if (find_page_in_stack(name) != nullptr) {
        PM_LOG_ERROR("Page(%s) was multi push", name);
        return false;
    }

    /* Check if the page is registered in the page pool */
    PageBase* base = find_page_in_pool(name);

    if (base == nullptr) {
        PM_LOG_ERROR("Page(%s) was not install", name);
        return false;
    }

    /* Get the top page of the stack */
    PageBase* top = get_stack_top();

    if (top == nullptr) {
        PM_LOG_ERROR("Stack top is NULL");
        return false;
    }

    /* Force disable cache */
    top->priv.is_cached = false;

    /* Synchronous automatic cache configuration */
    base->priv.is_disable_auto_cache = base->priv.req_disable_auto_cache;

    /* Remove current page */
    page_stack_.pop();

    /* Push into the stack */
    page_stack_.push(base);

    PM_LOG_INFO("Page(%s) replace Page(%s) (stash = 0x%p)", name, top->page_name, stash);

    /* Page switching execution */
    return switch_to(base, true, stash);
}

/**
  * @brief  Enter a new page, the old page is pushed onto the stack
  * @param  name: The name of the page to enter
  * @param  stash: Parameters passed to the new page
  * @retval Return true if successful
  */
bool PageManager::push(const char* name, const PageBase::Stash_t* stash) {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return false;
    }

    /* Check whether the stack is repeatedly pushed  */
    if (find_page_in_stack(name) != nullptr) {
        PM_LOG_ERROR("Page(%s) was multi push", name);
        return false;
    }

    /* Check if the page is registered in the page pool */
    PageBase* base = find_page_in_pool(name);

    if (base == nullptr) {
        PM_LOG_ERROR("Page(%s) was not install", name);
        return false;
    }

    /* Synchronous automatic cache configuration */
    base->priv.is_disable_auto_cache = base->priv.req_disable_auto_cache;

    /* Push into the stack */
    page_stack_.push(base);

    PM_LOG_INFO("Page(%s) push >> [Screen] (stash = 0x%p)", name, stash);

    /* Page switching execution */
    return switch_to(base, true, stash);
}

/**
  * @brief  Pop the current page
  * @param
  * @retval Return true if successful
  */
bool PageManager::pop() {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return false;
    }
    if (page_stack_.size() <= 1) {
        PM_LOG_WARN("Only root page remains, can't pop");
        return false;
    }
    /* Get the top page of the stack */
    PageBase* top = get_stack_top();

    if (top == nullptr) {
        PM_LOG_WARN("Page stack is empty, can't pop");
        return false;
    }

    /* Whether to turn off automatic cache */
    if (!top->priv.is_disable_auto_cache) {
        PM_LOG_INFO("Page(%s) has auto cache, cache disabled", top->page_name);
        top->priv.is_cached = false;
    }

    PM_LOG_INFO("Page(%s) pop << [Screen]", top->page_name);

    /* Page popup */
    if (!page_stack_.empty()) {
        page_stack_.pop();
    }

    /* Get the next page */
    top = get_stack_top();

    /* Page switching execution */
    return switch_to(top, false, nullptr);
}

/**
  * @brief  Page switching
  * @param  new_node: Pointer to new page
  * @param  is_enter_act: Whether it is a ENTER action
  * @param  stash: Parameters passed to the new page
  * @retval Return true if successful
  */
bool PageManager::switch_to(PageBase* new_node, const bool is_enter_act, const PageBase::Stash_t* stash) {
    if (new_node == nullptr) {
        PM_LOG_ERROR("newNode is nullptr");
        return false;
    }

    /* Whether page switching has been requested */
    if (anim_state_.is_switch_req) {
        PM_LOG_WARN("Page switch busy, require(%s) is ignore", new_node->page_name);
        return false;
    }

    anim_state_.is_switch_req = true;

    /* Is there a parameter to pass */
    if (stash != nullptr) {
        PM_LOG_INFO("stash is detect, %s >> stash(0x%p) >> %s", get_page_prev_name(), stash, new_node->page_name);

        if (stash->ptr == nullptr || stash->size == 0) {
            PM_LOG_ERROR("stash data is invalid");
        } else {
            void* buffer = nullptr;

            if (new_node->priv.stash.ptr != nullptr && new_node->priv.stash.size != stash->size) {
                PM_LOG_INFO("stash(0x%p) resize[%lu -> %lu]", new_node->priv.stash.ptr,
                            static_cast<unsigned long>(new_node->priv.stash.size),
                            static_cast<unsigned long>(stash->size));
                lv_mem_free(new_node->priv.stash.ptr);
                new_node->priv.stash.ptr = nullptr;
                new_node->priv.stash.size = 0;
            }

            if (new_node->priv.stash.ptr == nullptr) {
                buffer = lv_mem_alloc(stash->size);
                if (buffer == nullptr) {
                    PM_LOG_ERROR("stash malloc failed");
                } else {
                    PM_LOG_INFO("stash(0x%p) malloc[%lu]", buffer, static_cast<unsigned long>(stash->size));
                }
            } else if (new_node->priv.stash.size == stash->size) {
                buffer = new_node->priv.stash.ptr;
                PM_LOG_INFO("stash(0x%p) is exist", buffer);
            }

            if (buffer != nullptr) {
                memcpy(buffer, stash->ptr, stash->size);
                PM_LOG_INFO("stash memcpy[%lu] 0x%p >> 0x%p", static_cast<unsigned long>(stash->size), stash->ptr,
                            buffer);
                new_node->priv.stash.ptr = buffer;
                new_node->priv.stash.size = stash->size;
            }
        }
    }

    /* Record current page */
    page_current_ = new_node;

    /* If the current page has a cache */
    if (page_current_->priv.is_cached) {
        /* Direct display, no need to load */
        PM_LOG_INFO("Page(%s) has cached, appear directly", page_current_->page_name);
        page_current_->priv.state = PageBase::PAGE_STATE_WILL_APPEAR;
    } else {
        /* Load page */
        page_current_->priv.state = PageBase::PAGE_STATE_LOAD;
    }

    if (page_prev_ != nullptr) {
        page_prev_->priv.anim.is_enter = false;
    }

    page_current_->priv.anim.is_enter = true;

    anim_state_.is_entering = is_enter_act;

    if (anim_state_.is_entering) {
        /* Update the animation configuration according to the current page */
        switch_anim_type_update(page_current_);
    }

    /* Update the state machine of the previous page */
    state_update(page_prev_);

    /* Update the state machine of the current page */
    state_update(page_current_);

    /* Move the layer, move the new page to the front */
    if (anim_state_.is_entering) {
        PM_LOG_INFO("Page ENTER is detect, move Page(%s) to foreground", page_current_->page_name);
        if (page_prev_) {
            lv_obj_move_foreground(page_prev_->root);
        }
        lv_obj_move_foreground(page_current_->root);
    } else {
        PM_LOG_INFO("Page EXIT is detect, move Page(%s) to foreground", get_page_prev_name());
        lv_obj_move_foreground(page_current_->root);
        if (page_prev_) {
            lv_obj_move_foreground(page_prev_->root);
        }
    }
    return true;
}

/**
  * @brief  Force the end of the life cycle of the page without animation
  * @param  base: Pointer to the page being executed
  * @retval Return true if successful
  */
bool PageManager::force_unload(PageBase* base) {
    if (base == nullptr) {
        PM_LOG_ERROR("Page is nullptr, Unload failed");
        return false;
    }

    PM_LOG_INFO("Page(%s) Force unloading...", base->page_name);

    if (base->priv.state == PageBase::PAGE_STATE_ACTIVITY) {
        PM_LOG_INFO("Page state is ACTIVITY, Disappearing...");
        base->on_view_will_disappear();
        base->on_view_did_disappear();
    }
    base->priv.state = state_unload_execute(base);

    return true;
}

bool PageManager::reset_root(const char* name) {
    if (!switch_anim_state_check())
        return false;
    if (!name || !find_page_in_pool(name)) {
        PM_LOG_ERROR("Root page(%s) was not installed", name ? name : "NULL");
        return false;
    }
    set_stack_clear();
    page_current_ = page_prev_ = nullptr;
    return push(name);
}

/**
  * @brief  Back to the main page (the page at the bottom of the stack)
  * @param
  * @retval Return true if successful
  */
bool PageManager::back_home() {
    /* Check whether the animation of switching pages is being executed */
    if (!switch_anim_state_check()) {
        return false;
    }

    set_stack_clear(true);

    page_prev_ = nullptr;

    PageBase* home = get_stack_top();

    return switch_to(home, false);
}

/**
  * @brief  Check if the page switching animation is being executed
  * @param
  * @retval Return true if it is executing
  */
bool PageManager::switch_anim_state_check() const {
    if (anim_state_.is_switch_req || anim_state_.is_busy || anim_state_.is_dragging) {
        PM_LOG_WARN("Page switch busy[AnimState.IsSwitchReq = %d,"
                    "AnimState.IsBusy = %d,"
                    "AnimState.IsDragging = %d],"
                    "request ignored",
                    anim_state_.is_switch_req, anim_state_.is_busy, anim_state_.is_dragging);
        return false;
    }

    return true;
}

/**
  * @brief  Page switching request check
  * @param
  * @retval Return true if all pages are executed
  */
bool PageManager::switch_req_check() {
    bool ret = false;

    if (bool last_node_busy = page_prev_ && page_prev_->priv.anim.is_busy;
        !page_current_->priv.anim.is_busy && !last_node_busy) {
        PM_LOG_INFO("----Page switch was all finished----");
        anim_state_.is_switch_req = false;
        ret = true;
        page_prev_ = page_current_;
    } else {
        if (page_current_->priv.anim.is_busy) {
            PM_LOG_WARN("Page PageCurrent(%s) is busy", page_current_->page_name);
        } else {
            PM_LOG_WARN("Page PagePrev(%s) is busy", get_page_prev_name());
        }
    }

    return ret;
}

/**
  * @brief  PPage switching animation execution end callback
  * @param  a: Pointer to animation
  * @retval None
  */
void PageManager::on_switch_anim_finish(lv_anim_t* a) {
    auto* base = static_cast<PageBase*>(lv_anim_get_user_data(a));
    PageManager* manager = base->page_manager;

    PM_LOG_INFO("Page(%s) Anim finish", base->page_name);

    manager->state_update(base);
    base->priv.anim.is_busy = false;
    bool is_finished = manager->switch_req_check();

    if (!manager->anim_state_.is_entering && is_finished) {
        manager->switch_anim_type_update(manager->page_current_);
    }
}

/**
  * @brief  Create page switching animation
  * @param  base: Point to the animated page
  * @retval None
  */
void PageManager::switch_anim_create(PageBase* base) const {
    LoadAnimAttr_t anim_attr;
    if (!get_current_load_anim_attr(&anim_attr)) {
        return;
    }

    // A cached root may retain an offset from a different axis (e.g. horizontal menu -> vertical save -> home).
    // Restore properties not driven by this transition; keep its animated axis intact for drag/pop continuity.
    if (base->priv.anim.is_enter) {
        if (anim_attr.drag_dir != ROOT_DRAG_DIR_HOR)
            lv_obj_set_x(base->root, 0);
        if (anim_attr.drag_dir != ROOT_DRAG_DIR_VER)
            lv_obj_set_y(base->root, 0);
        if (get_current_load_anim_type() != LOAD_ANIM_FADE_ON)
            lv_obj_set_style_opa(base->root, LV_OPA_COVER, 0);
    }

    lv_anim_t a;
    anim_default_init(&a);
    lv_anim_set_user_data(&a, base);
    lv_anim_set_var(&a, base->root);
    lv_anim_set_ready_cb(&a, on_switch_anim_finish);
    lv_anim_set_exec_cb(&a, anim_attr.setter);

    int32_t start = 0;

    if (anim_attr.getter) {
        start = anim_attr.getter(base->root);
    }

    if (anim_state_.is_entering) {
        if (base->priv.anim.is_enter) {
            lv_anim_set_values(&a, anim_attr.push.enter.start, anim_attr.push.enter.end);
        } else /* Exit */
        {
            lv_anim_set_values(&a, start, anim_attr.push.exit.end);
        }
    } else /* Pop */
    {
        if (base->priv.anim.is_enter) {
            lv_anim_set_values(&a, anim_attr.pop.enter.start, anim_attr.pop.enter.end);
        } else /* Exit */
        {
            lv_anim_set_values(&a, start, anim_attr.pop.exit.end);
        }
    }

    lv_anim_start(&a);
    base->priv.anim.is_busy = true;
}

/**
  * @brief  Set global animation properties
  * @param  anim: Animation type
  * @param  time_ms: Animation duration
  * @param  path: Animation curve
  * @retval None
  */
void PageManager::set_global_load_anim_type(LoadAnim_t anim, uint16_t time_ms, lv_anim_path_cb_t path) {
    if (anim > LOAD_ANIM_LAST) {
        anim = LOAD_ANIM_NONE;
    }

    anim_state_.global.type = anim;
    anim_state_.global.time = time_ms;
    anim_state_.global.path = path != nullptr ? path : lv_anim_path_linear;

    PM_LOG_INFO("Set global load anim type = %d", anim);
}

/**
  * @brief  Update current animation properties, apply page custom animation
  * @param  base: Pointer to page
  * @retval None
  */
void PageManager::switch_anim_type_update(PageBase* base) {
    if (base->priv.anim.attr.type == LOAD_ANIM_GLOBAL) {
        PM_LOG_INFO("Page(%s) Anim.Type was not set, use AnimState.Global.Type = %d", base->page_name,
                    anim_state_.global.type);
        anim_state_.current = anim_state_.global;
    } else {
        if (base->priv.anim.attr.type > LOAD_ANIM_LAST) {
            PM_LOG_ERROR("Page(%s) ERROR custom Anim.Type = %d, use AnimState.Global.Type = %d", base->page_name,
                         base->priv.anim.attr.type, anim_state_.global.type);
            base->priv.anim.attr = anim_state_.global;
        } else {
            PM_LOG_INFO("Page(%s) custom Anim.Type set = %d", base->page_name, base->priv.anim.attr.type);
        }
        anim_state_.current = base->priv.anim.attr;
    }
}

/**
  * @brief  Set animation default parameters
  * @param  a: Pointer to animation
  * @retval None
  */
void PageManager::anim_default_init(lv_anim_t* a) const {
    lv_anim_init(a);

    const uint32_t duration = (get_current_load_anim_type() == LOAD_ANIM_NONE) ? 0 : anim_state_.current.time;
    lv_anim_set_time(a, duration);
    lv_anim_set_path_cb(a, anim_state_.current.path);
}
