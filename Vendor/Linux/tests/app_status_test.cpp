#include <cassert>
#include <cstring>
#include "DemoStatus.h"
#include "DomainTestInput.hpp"
#include "SystemService.h"
#include "UI/Common/PageStyle.h"
#include "UI/Resource/ResourcePool.h"
#include "UI/SaveConfig/SaveConfigModel.h"
#include "UI/SystemDash/SystemDashModel.h"
#include "UI/SystemInfos/SystemInfosModel.h"
#include "UI/SystemLoading/SystemLoadingModel.h"

/** @brief 通过公开 API 读取对象的屏幕绝对坐标供布局断言使用。 */
static lv_area_t
object_coords(const lv_obj_t* object) {
    lv_area_t area;
    lv_obj_get_coords(object, &area);
    return area;
}

static void advance(unsigned ms) {
    for (unsigned i = 0; i < ms; i += 10) {
        lv_tick_inc(10);
        DataProc_RunLoop();
        lv_timer_handler();
    }
}

static lv_obj_t* find_image(lv_obj_t* root, const void* source) {
    if (lv_obj_check_type(root, &lv_image_class) && lv_image_get_src(root) == source) {
        return root;
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(root); ++i) {
        if (auto* found = find_image(lv_obj_get_child(root, static_cast<int32_t>(i)), source))
            return found;
    }
    return nullptr;
}

static bool tinted(lv_obj_t* obj, lv_color_t color) {
    return lv_color_to_u32(lv_obj_get_style_image_recolor(obj, LV_PART_MAIN)) == lv_color_to_u32(color);
}

int main() {
    const auto start = DemoStatus::sample(0);
    assert(start.satellites == 8 && start.satellites_valid && start.clock_valid && start.hour == 12);
    const auto later = DemoStatus::sample(16000);
    assert(later.satellites == 9 && later.second == 16 && later.wifi && later.recording);
    const auto long_running = DemoStatus::sample(std::uint64_t{30} * 16 * 1000);
    assert(long_running.battery_percent >= 70 && long_running.battery_percent <= 85);
    assert(long_running.hour < 24 && long_running.minute < 60 && long_running.second < 60);
    lv_init();
    resource_pool::init();
    uint16_t pixels[294];
    lv_display_t* display = lv_display_create(294, 126);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t*, uint8_t*) { lv_display_flush_ready(d); });
    assert(display && DataProc_Init(lv_tick_get));
    assert(!DataProc_Init(lv_tick_get)); // No reset beneath live DataNodes.
    {
        page::StatusBar bar;
        assert(bar.create(lv_layer_top()));
        auto info = std::make_unique<page::SystemInfosModel>();
        assert(info->init());
        assert(DataProc::Broker()->search("SystemInfosModel"));
        assert(lv_obj_get_y(bar.root()) == -26 && !lv_obj_is_hidden(bar.root()));
        assert(info->set_status_bar(true));
        advance(600);
        assert(lv_obj_get_y(bar.root()) == 0);
        assert(info->set_status_bar(false, DataProc::StatusBarStyle::Transparent));
        advance(150);
        const auto hiding_y = lv_obj_get_y(bar.root());
        assert(hiding_y < 0 && hiding_y > -26);
        assert(!lv_obj_is_hidden(bar.root()));
        assert(lv_obj_get_style_bg_opa(bar.root(), LV_PART_MAIN) == LV_OPA_TRANSP);
        DataProc::StatusSnapshot data;
        std::strcpy(data.position.data(), "DEMO");
        data.battery_valid = true;
        data.battery_percent = 84;
        assert(test_input::status(data));
        data.position.fill('x');
        assert(!test_input::status(data)); // Reject an unterminated producer string.
        assert(info->status().battery_percent == 84 && std::strcmp(info->status().position.data(), "DEMO") == 0);
        assert(lv_obj_get_y(bar.root()) == hiding_y); // Publications cannot reveal or reposition the bar.
        assert(info->set_status_bar(false, DataProc::StatusBarStyle::Transparent));
        assert(lv_obj_get_y(bar.root()) == hiding_y); // Identical requests do not restart the animation.
        advance(400);
        assert(lv_obj_get_y(bar.root()) == -26 && object_coords(bar.root()).y2 < 0);
        DataNode peer("Peer", DataProc::Broker());
        assert(peer.subscribe("SystemInfosModel"));
        DataProc::StatusSnapshot pulled;
        assert(peer.pull("SystemInfosModel", &pulled, sizeof(pulled)) == DataNode::RES_OK);
        assert(std::strcmp(pulled.position.data(), "DEMO") == 0); // Model accepts a peer's pull request.
        assert(info->set_status_bar(true));
        advance(150);
        const auto showing_y = lv_obj_get_y(bar.root());
        assert(showing_y > -26 && showing_y < 0);
        assert(info->set_status_bar(false)); // Reverse mid-flight without snapping to a fixed endpoint.
        assert(lv_obj_get_y(bar.root()) == showing_y);
        advance(150);
        const auto reversing_y = lv_obj_get_y(bar.root());
        assert(reversing_y < showing_y);
        assert(info->set_status_bar(true));
        assert(lv_obj_get_y(bar.root()) == reversing_y);
        advance(600);
        assert(lv_obj_get_y(bar.root()) == 0 && object_coords(bar.root()).y1 == 0);
        assert(!lv_obj_is_hidden(bar.root()));
        assert(lv_obj_get_style_bg_opa(bar.root(), LV_PART_MAIN) == LV_OPA_COVER);
        // SD 图标：录制状态变化时先滑出，在栏外换色后滑回；录制中持续闪烁，停止后为静止白色。
        auto* sd = find_image(bar.root(), resource_pool::get_image("ui_sd_card"));
        assert(sd && tinted(sd, page::style::kText));
        data.position.fill('\0');
        std::strcpy(data.position.data(), "DEMO");
        data.recording = true;
        assert(test_input::status(data));
        advance(100);
        assert(lv_obj_get_style_translate_y(sd, LV_PART_MAIN) < 0 && tinted(sd, page::style::kText)); // 滑出期间不换色
        advance(400);
        assert(lv_obj_get_style_translate_y(sd, LV_PART_MAIN) == 0 && tinted(sd, page::style::kInfo));
        advance(300);
        assert(lv_obj_get_style_opa(sd, LV_PART_MAIN) < LV_OPA_COVER); // 闪烁
        data.recording = false;
        assert(test_input::status(data));
        assert(lv_obj_get_style_opa(sd, LV_PART_MAIN) == LV_OPA_COVER); // 停止闪烁后再滑出
        advance(500);
        assert(lv_obj_get_style_translate_y(sd, LV_PART_MAIN) == 0 && tinted(sd, page::style::kText));
        advance(1200);
        assert(lv_obj_get_style_opa(sd, LV_PART_MAIN) == LV_OPA_COVER); // 未录制不闪烁
        for (const bool recording : {true, false, true}) {   // 快速往返以最后一次上报为准
            data.recording = recording;
            assert(test_input::status(data));
            advance(60);
        }
        advance(600);
        assert(lv_obj_get_style_translate_y(sd, LV_PART_MAIN) == 0 && tinted(sd, page::style::kInfo));
        data.recording = false;
        assert(test_input::status(data));
        advance(600);
        assert(tinted(sd, page::style::kText) && lv_obj_get_style_opa(sd, LV_PART_MAIN) == LV_OPA_COVER);
        page::SystemDashModel settings;
        assert(settings.init());
        assert(!settings.settings().wifi_available && settings.settings().simulated);
        // The UI no longer exposes date editing; retain the shared RAM-service contract test.
        DataProc::SystemRequest time_request{};
        time_request.command = DataProc::SystemCommand::SetTime;
        time_request.date = {2026, 10, 1, 12, 30};
        assert(peer.subscribe("System"));
        assert(peer.notify("System", &time_request, sizeof(time_request)) == DataNode::RES_OK);
        // Startup results belong to System, survive loading-page teardown, and reach information Models.
        auto loading = std::make_unique<page::SystemLoadingModel>();
        assert(loading->init());
        assert(!info->settings().initialization_started);
        DataProc::SystemRequest boot_request{};
        boot_request.command = DataProc::SystemCommand::RecordInitialization;
        boot_request.initialization.status = DataProc::InitializationStatus::Ok;
        boot_request.command = DataProc::SystemCommand::BeginInitialization;
        assert(peer.notify("System", &boot_request, sizeof(boot_request)) == DataNode::RES_OK);
        assert(loading->begin_initialization() && info->settings().initialization_started);
        using Status = DataProc::InitializationStatus;
        assert(loading->poll_step(0, 0) == Status::Ok);
        // A running check (simulated 400 ms) reports Pending and records nothing until it finishes.
        assert(loading->poll_step(1, 0) == Status::Pending && loading->poll_step(1, 399) == Status::Pending);
        assert(info->settings().initialization[1].status == Status::NotRun);
        assert(loading->poll_step(1, 400) == Status::Ok);
        loading.reset(); // A partial attempt retains completed nodes and leaves later nodes NOT RUN.
        assert(info->settings().initialization[1].status == Status::Ok);
        assert(info->settings().initialization[2].status == Status::NotRun);
        loading = std::make_unique<page::SystemLoadingModel>();
        assert(loading->init() && loading->begin_initialization());
        for (unsigned i = 0; i < DataProc::kInitializationStepCount; ++i)
            assert(loading->poll_step(i, 2000) == (i != 3 ? Status::Ok : Status::Failed));
        assert(loading->poll_step(DataProc::kInitializationStepCount, 0) == Status::Failed);
        loading.reset();
        info.reset();
        info = std::make_unique<page::SystemInfosModel>();
        assert(info->init()); // A new information DataNode pulls the retained report after loading is gone.
        assert(!DataProc::Broker()->search("SystemLoadingModel"));
        assert(info->settings().initialization[3].status == DataProc::InitializationStatus::Failed);
        assert(std::strcmp(info->settings().initialization[3].detail.data(), "Wi-Fi backend reserved; continue offline")
               == 0);
        assert(info->settings().initialization[4].status == DataProc::InitializationStatus::Ok);
        loading = std::make_unique<page::SystemLoadingModel>();
        assert(loading->init());
        boot_request.command = DataProc::SystemCommand::RecordInitialization;
        boot_request.initialization_step = DataProc::kInitializationStepCount;
        assert(peer.notify("System", &boot_request, sizeof(boot_request)) == DataNode::RES_PARAM_ERROR);
        boot_request.initialization_step = 3;
        boot_request.initialization.status = static_cast<DataProc::InitializationStatus>(99);
        assert(peer.notify("System", &boot_request, sizeof(boot_request)) == DataNode::RES_PARAM_ERROR);
        boot_request.initialization.status = DataProc::InitializationStatus::Ok;
        boot_request.initialization.detail.fill('x');
        assert(peer.notify("System", &boot_request, sizeof(boot_request)) == DataNode::RES_PARAM_ERROR);
        assert(info->settings().initialization[3].status == DataProc::InitializationStatus::Failed);
        boot_request.initialization.detail.back() = '\0'; // The maximum terminated message is accepted.
        assert(peer.notify("System", &boot_request, sizeof(boot_request)) == DataNode::RES_OK);
        assert(std::strlen(info->settings().initialization[3].detail.data()) == 95);
        assert(loading->begin_initialization());
        for (const auto& node : info->settings().initialization)
            assert(node.status == DataProc::InitializationStatus::NotRun && node.detail[0] == '\0');
        loading.reset();
        assert(peer.notify("System", &time_request, sizeof(time_request)) == DataNode::RES_OK);
        assert(peer.unsubscribe("System"));
        assert(settings.settings().date.month == 10 && SystemService::snapshot().date.month == 10);
        assert(settings.toggle_language() && settings.settings().language == i18n::Language::Russian);
        assert(settings.toggle_language() && settings.settings().language == i18n::Language::English);
        assert(SystemService::valid_date({2000, 2, 29, 23, 59}));
        assert(!SystemService::valid_date({2025, 2, 29, 12, 30}));
        assert(!SystemService::valid_date({2026, 4, 31, 12, 30}));
        assert(settings.prepare_power(DataProc::PowerAction::Reboot));
        assert(SystemService::snapshot().power_pending && SystemService::snapshot().power_calls == 0);
        page::SaveConfigModel save;
        assert(save.init() && save.finish_power());
        assert(!SystemService::snapshot().power_pending && SystemService::snapshot().power_calls == 1);
        assert(save.finish_power() && SystemService::snapshot().power_calls == 1); // Duplicate request is idempotent.
        assert(SystemService::snapshot().power_action == DataProc::PowerAction::Reboot);
        assert(!DataProc_Deinit()); // Live page DataNodes prevent provider teardown.
        assert(info->set_status_bar(false));
        advance(100);
        auto* bar_root = bar.root();
        // 右侧分组最后两项为电池槽与电量数字。
        auto* battery_slot = lv_obj_get_child(lv_obj_get_child(bar_root, -1), -2);
        auto* battery_fill = lv_obj_get_child(battery_slot, 0);
        assert(lv_obj_get_width(battery_slot) == 24 && lv_obj_get_height(battery_slot) == 14);
        data.position.fill('\0');
        std::strcpy(data.position.data(), "DEMO");
        data.charging = true;
        assert(test_input::status(data) && lv_anim_get(battery_fill, nullptr));
        bar.destroy(); // No pending slide or charging animation may reference destroyed widgets.
        assert(!lv_anim_get(battery_fill, nullptr));
        assert(!lv_obj_is_in_widget_tree(bar_root));
        advance(600);
        assert(bar.create(lv_layer_top()) && lv_obj_get_y(bar.root()) == -26);
        battery_fill = lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(bar.root(), -1), -2), 0);
        assert(test_input::status(data) && lv_anim_get(battery_fill, nullptr));
        data.battery_valid = false;
        assert(test_input::status(data));
        advance(40);
        assert(!lv_anim_get(battery_fill, nullptr) && lv_obj_get_width(battery_fill) == 0);
        info.reset();
        assert(!DataProc::Broker()->search("SystemInfosModel"));
        assert(peer.getPublishersNumber() == 0);
    }
    assert(DataProc_Deinit() && DataProc::Broker()->getNodeNumber() == 0);
    assert(DataProc_Init(lv_tick_get) && DataProc_Deinit()); // Reinitialization after full cleanup.
    lv_display_delete(display);
}
