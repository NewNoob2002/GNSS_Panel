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
#include <algorithm>
#include "PM_Log.h"
#include "PageManager.h"

#define PM_EMPTY_PAGE_NAME "EMPTY_PAGE"

/**
  * @brief  Page manager constructor
  * @param  factory: Pointer to the page factory
  * @retval None
  */
PageManager::PageManager(PageFactory* factory)
    : pageFactory(factory), PagePrev(nullptr), PageCurrent(nullptr), RootDefaultStyle(nullptr) {
    lv_memset(&AnimState, 0, sizeof(AnimState));
    SetGlobalLoadAnimType();
}

/**
  * @brief  Page manager destructor
  * @param
  * @retval None
  */
PageManager::~PageManager() { Clear(); }

void
PageManager::Clear() {
    SetStackClear();

    for (PageBase* base : PagePool) {
        if (base->_root != nullptr) {
            ForceUnload(base);
        }
        delete base;
    }
    PagePool.clear();
    PagePrev = nullptr;
    PageCurrent = nullptr;
}

/**
  * @brief  Search pages in the page pool
  * @param  name: Page name
  * @retval A pointer to the base class of the page, or nullptr if not found
  */
PageBase*
PageManager::FindPageInPool(const char* name) const {
    for (const auto iter : PagePool) {
        if (strcmp(name, iter->pageName) == 0) {
            return iter;
        }
    }
    return nullptr;
}

/**
  * @brief  Search pages in the page stack
  * @param  name: Page name
  * @retval A pointer to the base class of the page, or nullptr if not found
  */
PageBase*
PageManager::FindPageInStack(const char* name) {
    decltype(PageStack) stk = PageStack;
    while (!stk.empty()) {
        if (PageBase* base = stk.top(); strcmp(name, base->pageName) == 0) {
            return base;
        }

        stk.pop();
    }

    return nullptr;
}

/**
  * @brief  Install the page, and register the page to the page pool
  * @param  className: The class name of the page
  * @param  appName: Page application name, no duplicates allowed
  * @retval Return true if successful
  */
bool
PageManager::Install(const char* className, const char* appName) {
    if (pageFactory == nullptr) {
        PM_LOG_ERROR("Factory was not registered, can't install page");
        return false;
    }

    if (appName == nullptr) {
        PM_LOG_WARN("appName has not set");
        appName = className;
    }

    if (FindPageInPool(appName) != nullptr) {
        PM_LOG_ERROR("Page(%s) was registered", appName);
        return false;
    }

    PageBase* base = pageFactory->CreatePage(className);
    if (base == nullptr) {
        PM_LOG_ERROR("Factory has not %s", className);
        return false;
    }

    base->_root = nullptr;
    base->pageID = 0;
    base->pageManager = nullptr;
    base->UserData = nullptr;
    lv_memset(&base->priv, 0, sizeof(base->priv));

    PM_LOG_INFO("Install Page[class = %s, name = %s]", className, appName);
    const bool retval = Register(base, appName);
    if (!retval) {
        delete base;
        return false;
    }

    base->onCustomAttrConfig();

    return true;
}

/**
  * @brief  Uninstall page
  * @param  appName: Page application name, no duplicates allowed
  * @retval Return true if the uninstallation is successful
  */
bool
PageManager::Uninstall(const char* appName) {
    PM_LOG_INFO("Page(%s) uninstall...", appName);

    PageBase* base = FindPageInPool(appName);
    if (base == nullptr) {
        PM_LOG_ERROR("Page(%s) was not found", appName);
        return false;
    }

    if (!Unregister(appName)) {
        PM_LOG_ERROR("Page(%s) unregister failed", appName);
        return false;
    }

    if (base->priv.IsCached) {
        PM_LOG_WARN("Page(%s) has cached, unloading...", appName);
        base->priv.State = PageBase::PAGE_STATE_UNLOAD;
        StateUpdate(base);
    } else {
        PM_LOG_INFO("Page(%s) has not cache", appName);
    }

    delete base;
    PM_LOG_INFO("Uninstall OK");
    return true;
}

/**
  * @brief  Register the page to the page pool
  * @param  base: Page Base Point
  * @param  name: Page application name, duplicate registration is not allowed
  * @retval Return true if the registration is successful
  */
bool
PageManager::Register(PageBase* base, const char* name) {
    if (FindPageInPool(name) != nullptr) {
        PM_LOG_ERROR("Page(%s) was multi registered", name);
        return false;
    }

    base->pageManager = this;
    base->pageName = name;
    PagePool.push_back(base);

    return true;
}

void
PageManager::NotifyLanguageChanged() const {
    for (PageBase* base : PagePool) {
        if (base != nullptr && base->_root != nullptr) {
            base->onLanguageChanged();
        }
    }
}

/**
  * @brief  Log out the page from the page pool
  * @param  name: Page application name
  * @retval Return true if the logout is successful
  */
bool
PageManager::Unregister(const char* name) {
    PM_LOG_INFO("Page(%s) unregister...", name);

    PageBase* base = FindPageInStack(name);

    if (base != nullptr) {
        PM_LOG_ERROR("Page(%s) was in stack", name);
        return false;
    }

    base = FindPageInPool(name);
    if (base == nullptr) {
        PM_LOG_ERROR("Page(%s) was not found", name);
        return false;
    }

    const auto iter = std::find(PagePool.begin(), PagePool.end(), base);

    if (iter == PagePool.end()) {
        PM_LOG_ERROR("Page(%s) was not found in PagePool", name);
        return false;
    }

    PagePool.erase(iter);

    PM_LOG_INFO("Unregister OK");
    return true;
}

/**
  * @brief  Get the top page of the page stack
  * @param
  * @retval A pointer to the base class of the page
  */
PageBase*
PageManager::GetStackTop() const {
    return PageStack.empty() ? nullptr : PageStack.top();
}

/**
  * @brief  Get the page below the top of the page stack
  * @param
  * @retval A pointer to the base class of the page
  */
PageBase*
PageManager::GetStackTopAfter() {
    PageBase* top = GetStackTop();

    if (top == nullptr) {
        return nullptr;
    }

    /* Remove current page */
    if (!PageStack.empty()) {
        PageStack.pop();
    }

    PageBase* topAfter = GetStackTop();

    PageStack.push(top);

    return topAfter;
}

/**
  * @brief  Clear the page stack and end the life cycle of all pages in the page stack
  * @param  keepBottom: Whether to keep the bottom page of the stack
  * @retval None
  */
void
PageManager::SetStackClear(const bool keepBottom) {
    while (true) {
        PageBase* top = GetStackTop();

        if (top == nullptr) {
            PM_LOG_INFO("Page stack is empty, breaking...");
            break;
        }

        if (const PageBase* topAfter = GetStackTopAfter(); topAfter == nullptr) {
            if (keepBottom) {
                PagePrev = top;
                PM_LOG_INFO("Keep page stack bottom(%s), breaking...", top->pageName);
                break;
            }
            PagePrev = nullptr;
        }

        ForceUnload(top);

        /* Remove current page */
        PageStack.pop();
    }
    PM_LOG_INFO("Stack clear done");
}

/**
  * @brief  Get the name of the previous page
  * @param
  * @retval The name of the previous page, if it does not exist, return PM_EMPTY_PAGE_NAME
  */
const char*
PageManager::GetPagePrevName() const {
    return PagePrev ? PagePrev->pageName : PM_EMPTY_PAGE_NAME;
}
