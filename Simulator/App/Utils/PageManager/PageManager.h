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
#ifndef PAGE_MANAGER_H
#define PAGE_MANAGER_H

#include <stack>
#include <vector>
#include "PageBase.h"
#include "PageFactory.h"

class PageManager {
  public:
    /* Page switching animation type  */
    typedef enum {
        /* Default (global) animation type  */
        LOAD_ANIM_GLOBAL = 0,

        /* New page overwrites old page  */
        LOAD_ANIM_OVER_LEFT,
        LOAD_ANIM_OVER_RIGHT,
        LOAD_ANIM_OVER_TOP,
        LOAD_ANIM_OVER_BOTTOM,

        /* New page pushes old page  */
        LOAD_ANIM_MOVE_LEFT,
        LOAD_ANIM_MOVE_RIGHT,
        LOAD_ANIM_MOVE_TOP,
        LOAD_ANIM_MOVE_BOTTOM,

        /* The new interface fades in, the old page fades out */
        LOAD_ANIM_FADE_ON,

        /* No animation */
        LOAD_ANIM_NONE,

        LOAD_ANIM_LAST = LOAD_ANIM_NONE
    } LoadAnim_t;

    /* Page dragging direction */
    typedef enum {
        ROOT_DRAG_DIR_NONE,
        ROOT_DRAG_DIR_HOR,
        ROOT_DRAG_DIR_VER,
    } RootDragDir_t;

    /* Animated setter */
    typedef void (*lv_anim_setter_t)(void*, int32_t);

    /* Animated getter */
    typedef int32_t (*lv_anim_getter_t)(void*);

    /* Animation switching record  */
    typedef struct {
        /* As the entered party */
        struct {
            int32_t start;
            int32_t end;
        } enter;

        /* As the exited party */
        struct {
            int32_t start;
            int32_t end;
        } exit;
    } AnimValue_t;

    /* Page loading animation properties */
    typedef struct {
        lv_anim_setter_t setter;
        lv_anim_getter_t getter;
        RootDragDir_t drag_dir;
        AnimValue_t push;
        AnimValue_t pop;
    } LoadAnimAttr_t;

  public:
    explicit PageManager(PageFactory* factory = nullptr);
    ~PageManager();

    /* Loader */
    bool install(const char* class_name, const char* app_name);
    bool uninstall(const char* app_name);
    bool register_page(PageBase* base, const char* name);
    bool unregister_page(const char* name);

    /* Router */
    bool replace(const char* name, const PageBase::Stash_t* stash = nullptr);
    bool push(const char* name, const PageBase::Stash_t* stash = nullptr);
    bool pop();
    bool back_home();
    /** Unload the navigation stack and load name as its sole root on the UI thread.
     * Only valid outside switching/dragging; null/unregistered names or busy state return false unchanged.
     * Loaded page views are destroyed synchronously, including the caller's view; registered controllers remain.
     */
    bool reset_root(const char* name);
    [[nodiscard]] const char* get_page_prev_name() const;

    /* Global Animation */
    void set_global_load_anim_type(LoadAnim_t anim = LOAD_ANIM_OVER_LEFT, uint16_t time_ms = 500,
                                   lv_anim_path_cb_t path = lv_anim_path_ease_out);

    void set_root_default_style(lv_style_t* style) {
        root_default_style_ = style;
    }

    PageBase* get_current_page() const {
        return page_current_;
    }

    bool is_switching() const {
        return anim_state_.is_busy || anim_state_.is_switch_req;
    }
    void notify_language_changed() const;

  private:
    /* Page Pool */
    PageBase* find_page_in_pool(const char* name) const;

    /* Page Stack */
    PageBase* find_page_in_stack(const char* name);
    [[nodiscard]] PageBase* get_stack_top() const;
    PageBase* get_stack_top_after();
    void set_stack_clear(bool keep_bottom = false);
    static bool force_unload(PageBase* base);

    /* Animation */
    static bool get_load_anim_attr(uint8_t anim, LoadAnimAttr_t* attr);

    static bool get_is_over_anim(const uint8_t anim) {
        return (anim >= LOAD_ANIM_OVER_LEFT && anim <= LOAD_ANIM_OVER_BOTTOM);
    }

    static bool get_is_move_anim(const uint8_t anim) {
        return (anim >= LOAD_ANIM_MOVE_LEFT && anim <= LOAD_ANIM_MOVE_BOTTOM);
    }

    void anim_default_init(lv_anim_t* a) const;

    bool get_current_load_anim_attr(LoadAnimAttr_t* attr) const {
        return get_load_anim_attr(get_current_load_anim_type(), attr);
    }

    [[nodiscard]] LoadAnim_t get_current_load_anim_type() const {
        return static_cast<LoadAnim_t>(anim_state_.current.type);
    }

    /* Root */
    static void on_root_drag_event(lv_event_t* event);
    static void on_root_drag_anim_finish(lv_anim_t* a);
    static void on_root_async_leave(void* base);
    static void root_enable_drag(lv_obj_t* root);
    static void root_get_drag_predict(lv_coord_t* x, lv_coord_t* y);

    /* Switch */
    bool switch_to(PageBase* new_node, bool is_enter_act, const PageBase::Stash_t* stash = nullptr);
    static void on_switch_anim_finish(lv_anim_t* a);

    void switch_anim_create(PageBase* base) const;
    void switch_anim_type_update(PageBase* base);
    bool switch_req_check();
    [[nodiscard]] bool switch_anim_state_check() const;

    /* State */
    PageBase::State_t state_load_execute(PageBase* base);
    PageBase::State_t state_will_appear_execute(PageBase* base) const;
    static PageBase::State_t state_did_appear_execute(PageBase* base);
    PageBase::State_t state_will_disappear_execute(PageBase* base) const;
    static PageBase::State_t state_did_disappear_execute(PageBase* base);
    static PageBase::State_t state_unload_execute(PageBase* base);
    void state_update(PageBase* base);

    [[nodiscard]] PageBase::State_t get_state() const {
        return page_current_->priv.state;
    }

    /* Page factory */
    PageFactory* page_factory_;

    /* Page pool */
    std::vector<PageBase*> page_pool_;

    /* Page stack */
    std::stack<PageBase*> page_stack_;

    /* Previous page */
    PageBase* page_prev_;

    /* The current page */
    PageBase* page_current_;

    /* Page animation status */
    struct {
        bool is_switch_req; // Has switch request
        bool is_busy;       // Is switching
        bool is_entering;   // Is in entering action
        bool is_dragging;   // Is handling a root drag gesture

        PageBase::AnimAttr_t current; // Current animation properties
        PageBase::AnimAttr_t global;  // Global animation properties
    } anim_state_{};

    /* Root style */
    lv_style_t* root_default_style_;
};

#endif
