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
#include "PageBase.h"
#include "PM_Log.h"

void PageBase::set_custom_cache_enable(const bool en) {
    PM_LOG_INFO("Page(%s) %s = %d", page_name, __func__, en);
    set_custom_auto_cache_enable(false);
    priv.req_enable_cache = en;
}

void PageBase::set_custom_auto_cache_enable(const bool en) {
    PM_LOG_INFO("Page(%s) %s = %d", page_name, __func__, en);
    priv.req_disable_auto_cache = !en;
}

void PageBase::set_custom_load_anim_type(const uint8_t anim_type, const uint16_t time, const lv_anim_path_cb_t path) {
    priv.anim.attr.type = anim_type;
    priv.anim.attr.time = time;
    priv.anim.attr.path = path != nullptr ? path : lv_anim_path_linear;
}

bool PageBase::stash_pop(void* ptr, const uint32_t size) {
    if (ptr == nullptr || size == 0) {
        PM_LOG_WARN("Invalid Stash destination");
        return false;
    }

    if (priv.stash.ptr == nullptr) {
        PM_LOG_WARN("No Stash found");
        return false;
    }

    if (priv.stash.size != size) {
        PM_LOG_WARN("Stash[0x%p](%lu) does not match the size(%lu)", priv.stash.ptr,
                    static_cast<unsigned long>(priv.stash.size), static_cast<unsigned long>(size));
        return false;
    }

    lv_memcpy(ptr, priv.stash.ptr, priv.stash.size);
    lv_free(priv.stash.ptr);
    priv.stash.ptr = nullptr;
    priv.stash.size = 0;
    return true;
}
