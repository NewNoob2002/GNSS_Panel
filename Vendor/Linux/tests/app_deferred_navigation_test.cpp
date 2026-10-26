#include <cassert>
#include <cstdio>
#include "App.h"
#include "Def/DP_PageNavi.h"
#include "Def/DP_Power.h"
#include "DemoModel.h"
#include "UI/PageId.h"

namespace {
uint16_t pixels[294 * 20];

void
flush(lv_display_t* display, const lv_area_t*, uint8_t*) {
    lv_display_flush_ready(display);
}

void
advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        lv_timer_handler();
    }
}

struct Observer {
    App& app;
    bool inside_notify = false, nested = false;
    unsigned callbacks = 0;
};

int
receive(DataNode* caller, DataNode::EventParam_t* event) {
    auto& observer = *static_cast<Observer*>(caller->getUserData());
    if (observer.inside_notify) {
        /** @brief 同步 publish/notify 链中，页面必须仍为调用前的 Startup。 */
        assert(page::id(observer.app.current_page()) == page::PageId::Startup);
        ++observer.callbacks;
        dp::PowerState power;
        if (dp::Power::receive(event, power) == DataNode::RES_OK && observer.nested) {
            observer.nested = false;
            assert(dp::PageNavi::notify(*caller,
                                        {dp::PageNaviOperation::Push, static_cast<uint16_t>(page::PageId::SystemInfos)})
                   == DataNode::RES_OK);
            assert(page::id(observer.app.current_page()) == page::PageId::Startup);
        }
    }
    return DataNode::RES_OK;
}
} // namespace

int
main() {
    lv_init();
    auto* display = lv_display_create(294, 126);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    for (bool power : {false, true}) {
        demo::model().power = {};
        App app;
        assert(app.init());
        advance(400);
        Observer observer{app};
        DataNode caller("DeferredNavigationTest", DataProc::Broker(), &observer);
        caller.setEventCallback(receive);
        assert(dp::PageNavi::subscribe(caller) && dp::Power::subscribe(caller));
        observer.inside_notify = true;
        if (power) {
            observer.nested = true;
            assert(dp::Power::notify(caller, {dp::PowerOperation::RequestOn}) == DataNode::RES_OK);
            assert(observer.callbacks >= 2);
        } else {
            assert(dp::PageNavi::notify(caller,
                                        {dp::PageNaviOperation::Push, static_cast<uint16_t>(page::PageId::SystemInfos)})
                   == DataNode::RES_OK);
            assert(observer.callbacks == 1);
        }
        observer.inside_notify = false;
        assert(page::id(app.current_page()) == page::PageId::Startup);
        advance(20);
        assert(page::id(app.current_page()) == (power ? page::PageId::SystemLoading : page::PageId::SystemInfos));
        if (power) {
            /** @brief Power 优先，嵌套请求等待切换完成后的另一个定时节拍。 */
            advance(200);
            assert(page::id(app.current_page()) == page::PageId::SystemLoading);
            advance(200);
            assert(page::id(app.current_page()) == page::PageId::SystemInfos);
        }
        advance(400);
        assert(app.show_page(page::name(page::PageId::WorkSettings)));
        assert(page::id(app.current_page()) == page::PageId::WorkSettings);
        advance(400);
        assert(app.back());
        assert(page::id(app.current_page()) == page::PageId::SystemInfos);
    }
    lv_display_delete(display);
    std::puts("PageNavi/Power：同步回调不卸载页面，下一 UI 节拍才导航；平台预览接口保留同步语义。");
}
