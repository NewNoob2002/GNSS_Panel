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
    : page_factory_(factory), page_prev_(nullptr), page_current_(nullptr), root_default_style_(nullptr) {
    lv_memset(&anim_state_, 0, sizeof(anim_state_));
    set_global_load_anim_type();
}

/**
  * @brief  Page manager destructor
  * @param
  * @retval None
  */
PageManager::~PageManager() {
    set_stack_clear();

    for (PageBase* base : page_pool_) {
        if (base->root != nullptr) {
            force_unload(base);
        }
        delete base;
    }
    page_pool_.clear();
}

/**
  * @brief  Search pages in the page pool
  * @param  name: Page name
  * @retval A pointer to the base class of the page, or nullptr if not found
  */
PageBase* PageManager::find_page_in_pool(const char* name) const {
    if (name == nullptr)
        return nullptr;

    for (const auto iter : page_pool_) {
        if (strcmp(name, iter->page_name) == 0) {
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
PageBase* PageManager::find_page_in_stack(const char* name) {
    if (name == nullptr)
        return nullptr;

    decltype(page_stack_) stk = page_stack_;
    while (!stk.empty()) {
        if (PageBase* base = stk.top(); strcmp(name, base->page_name) == 0) {
            return base;
        }

        stk.pop();
    }

    return nullptr;
}

/**
  * @brief  Install the page, and register the page to the page pool
  * @param  class_name: The class name of the page
  * @param  app_name: Page application name, no duplicates allowed
  * @retval Return true if successful
  */
bool PageManager::install(const char* class_name, const char* app_name) {
    if (page_factory_ == nullptr || class_name == nullptr) {
        PM_LOG_ERROR("Factory was not registered, can't install page");
        return false;
    }

    if (app_name == nullptr) {
        PM_LOG_WARN("app_name has not set");
        app_name = class_name;
    }

    if (find_page_in_pool(app_name) != nullptr) {
        PM_LOG_ERROR("Page(%s) was registered", app_name);
        return false;
    }

    PageBase* base = page_factory_->create_page(class_name);
    if (base == nullptr) {
        PM_LOG_ERROR("Factory has not %s", class_name);
        return false;
    }

    base->root = nullptr;
    base->page_id = 0;
    base->page_manager = nullptr;
    base->user_data = nullptr;
    lv_memset(&base->priv, 0, sizeof(base->priv));

    PM_LOG_INFO("Install Page[class = %s, name = %s]", class_name, app_name);
    const bool retval = register_page(base, app_name);
    if (!retval) {
        delete base;
        return false;
    }

    base->on_custom_attr_config();

    return true;
}

/**
  * @brief  Uninstall page
  * @param  app_name: Page application name, no duplicates allowed
  * @retval Return true if the uninstallation is successful
  */
bool PageManager::uninstall(const char* app_name) {
    if (app_name == nullptr)
        return false;

    PM_LOG_INFO("Page(%s) uninstall...", app_name);

    PageBase* base = find_page_in_pool(app_name);
    if (base == nullptr) {
        PM_LOG_ERROR("Page(%s) was not found", app_name);
        return false;
    }

    if (!unregister_page(app_name)) {
        PM_LOG_ERROR("Page(%s) unregister failed", app_name);
        return false;
    }

    if (base->priv.is_cached) {
        PM_LOG_WARN("Page(%s) has cached, unloading...", app_name);
        base->priv.state = PageBase::PAGE_STATE_UNLOAD;
        state_update(base);
    } else {
        PM_LOG_INFO("Page(%s) has not cache", app_name);
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
bool PageManager::register_page(PageBase* base, const char* name) {
    if (base == nullptr || name == nullptr)
        return false;

    if (find_page_in_pool(name) != nullptr) {
        PM_LOG_ERROR("Page(%s) was multi registered", name);
        return false;
    }

    base->page_manager = this;
    base->page_name = name;
    page_pool_.push_back(base);

    return true;
}

void PageManager::notify_language_changed() const {
    for (PageBase* base : page_pool_) {
        if (base != nullptr && base->root != nullptr) {
            base->on_language_changed();
        }
    }
}

/**
  * @brief  Log out the page from the page pool
  * @param  name: Page application name
  * @retval Return true if the logout is successful
  */
bool PageManager::unregister_page(const char* name) {
    if (name == nullptr)
        return false;

    PM_LOG_INFO("Page(%s) unregister...", name);

    PageBase* base = find_page_in_stack(name);

    if (base != nullptr) {
        PM_LOG_ERROR("Page(%s) was in stack", name);
        return false;
    }

    base = find_page_in_pool(name);
    if (base == nullptr) {
        PM_LOG_ERROR("Page(%s) was not found", name);
        return false;
    }

    const auto iter = std::find(page_pool_.begin(), page_pool_.end(), base);

    if (iter == page_pool_.end()) {
        PM_LOG_ERROR("Page(%s) was not found in PagePool", name);
        return false;
    }

    page_pool_.erase(iter);

    PM_LOG_INFO("Unregister OK");
    return true;
}

/**
  * @brief  Get the top page of the page stack
  * @param
  * @retval A pointer to the base class of the page
  */
PageBase* PageManager::get_stack_top() const {
    return page_stack_.empty() ? nullptr : page_stack_.top();
}

/**
  * @brief  Get the page below the top of the page stack
  * @param
  * @retval A pointer to the base class of the page
  */
PageBase* PageManager::get_stack_top_after() {
    PageBase* top = get_stack_top();

    if (top == nullptr) {
        return nullptr;
    }

    /* Remove current page */
    if (!page_stack_.empty()) {
        page_stack_.pop();
    }

    PageBase* top_after = get_stack_top();

    page_stack_.push(top);

    return top_after;
}

/**
  * @brief  Clear the page stack and end the life cycle of all pages in the page stack
  * @param  keep_bottom: Whether to keep the bottom page of the stack
  * @retval None
  */
void PageManager::set_stack_clear(const bool keep_bottom) {
    while (true) {
        PageBase* top = get_stack_top();

        if (top == nullptr) {
            PM_LOG_INFO("Page stack is empty, breaking...");
            break;
        }

        if (const PageBase* top_after = get_stack_top_after(); top_after == nullptr) {
            if (keep_bottom) {
                page_prev_ = top;
                PM_LOG_INFO("Keep page stack bottom(%s), breaking...", top->page_name);
                break;
            }
            page_prev_ = nullptr;
        }

        force_unload(top);

        /* Remove current page */
        page_stack_.pop();
    }
    PM_LOG_INFO("Stack clear done");
}

/**
  * @brief  Get the name of the previous page
  * @param
  * @retval The name of the previous page, if it does not exist, return PM_EMPTY_PAGE_NAME
  */
const char* PageManager::get_page_prev_name() const {
    return page_prev_ ? page_prev_->page_name : PM_EMPTY_PAGE_NAME;
}
