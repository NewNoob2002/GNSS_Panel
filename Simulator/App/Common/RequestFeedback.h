#pragma once
#include "Utils/I18n/I18n.h"
#include "lvgl.h"

namespace page {
// 确认只表示后端接收请求，不代表物理操作已经完成。
struct RequestFeedback {
    i18n::TextId text = i18n::TextId::Count;
    uint32_t since = 0;
    void begin() {
        text = i18n::TextId::RequestWaiting;
        since = lv_tick_get();
    }
    void finish(bool accepted, bool pending) {
        text = !accepted ? i18n::TextId::RequestRejected
               : pending ? i18n::TextId::RequestWaiting
                         : i18n::TextId::RequestReceived;
    }
    void poll(bool pending) {
        if (text != i18n::TextId::RequestWaiting && text != i18n::TextId::RequestTimeout)
            return;
        if (!pending)
            text = i18n::TextId::RequestReceived;
        else if (lv_tick_elaps(since) >= 15000)
            text = i18n::TextId::RequestTimeout;
    }
};
} // namespace page
