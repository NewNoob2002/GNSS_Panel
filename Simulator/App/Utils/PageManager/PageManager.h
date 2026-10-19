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
        RootDragDir_t dragDir;
        AnimValue_t push;
        AnimValue_t pop;
    } LoadAnimAttr_t;

  public:
    explicit PageManager(PageFactory* factory = nullptr);
    ~PageManager();
    void Clear();

    /* Loader */
    bool Install(const char* className, const char* appName);
    bool Uninstall(const char* appName);
    bool Register(PageBase* base, const char* name);
    bool Unregister(const char* name);

    /* Router */
    bool Replace(const char* name, const PageBase::Stash_t* stash = nullptr);
    bool Push(const char* name, const PageBase::Stash_t* stash = nullptr);
    bool Pop();
    bool BackHome();
    [[nodiscard]] const char* GetPagePrevName() const;

    /* Global Animation */
    void SetGlobalLoadAnimType(LoadAnim_t anim = LOAD_ANIM_OVER_LEFT, uint16_t time = 500,
                               lv_anim_path_cb_t path = lv_anim_path_ease_out);

    void
    SetRootDefaultStyle(lv_style_t* style) {
        RootDefaultStyle = style;
    }

    PageBase*
    GetCurrentPage() const {
        return PageCurrent;
    }

    void NotifyLanguageChanged() const;

  private:
    /* Page Pool */
    PageBase* FindPageInPool(const char* name) const;

    /* Page Stack */
    PageBase* FindPageInStack(const char* name);
    [[nodiscard]] PageBase* GetStackTop() const;
    PageBase* GetStackTopAfter();
    void SetStackClear(bool keepBottom = false);
    static bool ForceUnload(PageBase* base);

    /* Animation */
    static bool GetLoadAnimAttr(uint8_t anim, LoadAnimAttr_t* attr);

    static bool
    GetIsOverAnim(const uint8_t anim) {
        return (anim >= LOAD_ANIM_OVER_LEFT && anim <= LOAD_ANIM_OVER_BOTTOM);
    }

    static bool
    GetIsMoveAnim(const uint8_t anim) {
        return (anim >= LOAD_ANIM_MOVE_LEFT && anim <= LOAD_ANIM_MOVE_BOTTOM);
    }

    void AnimDefaultInit(lv_anim_t* a) const;

    bool
    GetCurrentLoadAnimAttr(LoadAnimAttr_t* attr) const {
        return GetLoadAnimAttr(GetCurrentLoadAnimType(), attr);
    }

    [[nodiscard]] LoadAnim_t
    GetCurrentLoadAnimType() const {
        return static_cast<LoadAnim_t>(AnimState.Current.Type);
    }

    /* Root */
    static void onRootDragEvent(lv_event_t* event);
    static void onRootDragAnimFinish(lv_anim_t* a);
    static void onRootAsyncLeave(void* base);
    static void RootEnableDrag(lv_obj_t* root);
    static void RootGetDragPredict(lv_coord_t* x, lv_coord_t* y);

    /* Switch */
    bool SwitchTo(PageBase* newNode, bool isEnterAct, const PageBase::Stash_t* stash = nullptr);
    static void onSwitchAnimFinish(lv_anim_t* a);

    void SwitchAnimCreate(PageBase* base) const;
    void SwitchAnimTypeUpdate(PageBase* base);
    bool SwitchReqCheck();
    [[nodiscard]] bool SwitchAnimStateCheck() const;

    /* State */
    PageBase::State_t StateLoadExecute(PageBase* base);
    PageBase::State_t StateWillAppearExecute(PageBase* base) const;
    static PageBase::State_t StateDidAppearExecute(PageBase* base);
    PageBase::State_t StateWillDisappearExecute(PageBase* base) const;
    static PageBase::State_t StateDidDisappearExecute(PageBase* base);
    static PageBase::State_t StateUnloadExecute(PageBase* base);
    void StateUpdate(PageBase* base);

    [[nodiscard]] PageBase::State_t
    GetState() const {
        return PageCurrent->priv.State;
    }

    /* Page factory */
    PageFactory* pageFactory;

    /* Page pool */
    std::vector<PageBase*> PagePool;

    /* Page stack */
    std::stack<PageBase*> PageStack;

    /* Previous page */
    PageBase* PagePrev;

    /* The current page */
    PageBase* PageCurrent;

    /* Page animation status */
    struct {
        bool IsSwitchReq; // Has switch request
        bool IsBusy;      // Is switching
        bool IsEntering;  // Is in entering action
        bool IsDragging;  // Is handling a root drag gesture

        PageBase::AnimAttr_t Current; // Current animation properties
        PageBase::AnimAttr_t Global;  // Global animation properties
    } AnimState{};

    /* Root style */
    lv_style_t* RootDefaultStyle;
};

#endif
