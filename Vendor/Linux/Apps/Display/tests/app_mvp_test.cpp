#include <cassert>
#include <cstdio>
#include <cstring>
#include <memory>
#include "DemoModel.h"
#include "UI/BatteryWarning/BatteryWarning.h"
#include "UI/Common/PageStyle.h"
#include "UI/Dialplate/DialplateModel.h"
#include "UI/Dialplate/DialplateView.h"
#include "UI/RecordConfig/RecordConfigModel.h"
#include "UI/Resource/ResourcePool.h"
#include "UI/SaveConfig/SaveConfigModel.h"
#include "UI/StarMap/StarMapModel.h"
#include "UI/Startup/StartupModel.h"
#include "UI/StatusBar/StatusBar.h"
#include "UI/SystemDash/SystemDashModel.h"
#include "UI/SystemInfos/SystemInfosModel.h"
#include "UI/SystemLoading/SystemLoadingModel.h"
#include "UI/WorkSettings/WorkSettingsModel.h"

namespace {
void
count_change(lv_observer_t* observer, lv_subject_t*) {
    ++*static_cast<unsigned*>(lv_observer_get_user_data(observer));
}

template <class Domain>
void
check_subscription(DataNode& model) {
    typename Domain::State value;
    assert(Domain::pull(model, value) == DataNode::RES_OK);
}

template <class State>
void
publish(const char* name, const State& value) {
    auto* node = DataProc::Broker()->search(name);
    assert(node && node->publish(&value, sizeof(value)) == DataNode::RES_OK);
}

lv_obj_t*
position_label(const page::StatusBar& bar) {
    return lv_obj_get_child(lv_obj_get_child(bar.root(), 0), -1);
}

lv_obj_t*
battery_fill(const page::StatusBar& bar) {
    return lv_obj_get_child(lv_obj_get_child(lv_obj_get_child(bar.root(), -1), -2), 0);
}

/** @brief 不推进 LVGL 或服务时钟，创建与重建都必须立即显示 pull 到的有效值。 */
void
check_initial(const page::StatusBar& bar, unsigned percent, const char* position) {
    auto* model = static_cast<page::StatusBarModel*>(DataProc::Broker()->search("StatusBar"));
    assert(model && model->state().battery_percent == percent && model->state().battery_valid);
    assert(model->state().satellites_valid && model->state().satellites == demo::model().gnss.satellites_used);
    assert(model->state().clock_valid && model->state().hour == demo::model().clock.hour);
    assert(std::strcmp(lv_label_get_text(position_label(bar)), position) == 0);
    auto* numbers = lv_obj_get_child(lv_obj_get_child(bar.root(), -1), -1);
    assert(!lv_obj_is_hidden(numbers) && lv_anim_get(battery_fill(bar), nullptr));
    assert(lv_obj_get_y(bar.root()) == -26);
}
} // namespace

int
main() {
    lv_init();
    resource_pool::init();
    auto* display = lv_display_create(294, 126);
    uint16_t pixels[294];
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels, nullptr, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t*, uint8_t*) { lv_display_flush_ready(d); });

    auto& input = demo::model();
    input.board.capabilities.simulated = false;
    input.battery.level_percent = 47;
    input.battery.temperature = 67;
    input.battery.charge = hal::ChargeState::Fast;
    input.gnss.satellites_used = 23;
    input.gnss.fix = hal::FixStatus::Fixed;
    input.clock.hour = 7;
    input.wifi.on = true;
    input.recorder.recording = true;
    input.recorder.reported_recording = false;
    assert(DataProc_Init(lv_tick_get));
    {
        page::StatusBar bar;
        page::BatteryWarning warning;
        assert(bar.create(lv_layer_top()) && warning.create(lv_layer_top()) && warning.active());
        check_initial(bar, 47, i18n::text(i18n::TextId::PositionFix));
        auto* model = static_cast<page::StatusBarModel*>(DataProc::Broker()->search("StatusBar"));
        assert(model->getPublishersNumber() == 8);
        check_subscription<dp::Battery>(*model);
        check_subscription<dp::Clock>(*model);
        check_subscription<dp::GNSS>(*model);
        check_subscription<dp::Recorder>(*model);
        check_subscription<dp::Settings>(*model);
        check_subscription<dp::Version>(*model);
        check_subscription<dp::WiFi>(*model);
        check_subscription<dp::Work>(*model);
        auto* warning_model = static_cast<page::BatteryWarningModel*>(DataProc::Broker()->search("BatteryWarning"));
        assert(warning_model->getPublishersNumber() == 2);
        check_subscription<dp::Battery>(*warning_model);
        check_subscription<dp::Settings>(*warning_model);
        assert(!model->state().recording && model->state().wifi);

        /** @brief 没有 AppNavigator 喂语言时，两组件也必须直接响应 Settings。 */
        {
            DataNode caller("LanguageProbe", DataProc::Broker());
            assert(dp::Settings::subscribe(caller));
            auto* label = lv_obj_get_child(lv_obj_get_child(lv_layer_top(), -1), 1);
            assert(lv_obj_check_type(label, &lv_label_class));
            for (auto language : {i18n::Language::Russian, i18n::Language::English}) {
                assert(dp::Settings::notify(caller, {language}) == DataNode::RES_OK);
                assert(warning_model->state().language == language && warning.active());
                char expected[128];
                std::snprintf(expected, sizeof(expected), i18n::text(i18n::TextId::BatteryTemperatureCritical), "67.0");
                assert(std::strcmp(lv_label_get_text(label), expected) == 0);
                assert(std::strcmp(lv_label_get_text(position_label(bar)), i18n::text(i18n::TextId::PositionFix)) == 0);
            }
        }

        unsigned changes = 0;
        auto* observer = lv_subject_add_observer(model->subject(), count_change, &changes);
        assert(observer && changes == 1);
        /** @brief 旧聚合已删除；无关领域与相关领域的非显示字段都不刷新状态栏。 */
        for (const char* name : {"Status", "System", "Device"}) {
            assert(!DataProc::Broker()->search(name));
            assert(!model->subscribe(name));
        }
        auto version = input.board;
        ++version.uptime_s;
        publish(dp::names::Version, version);
        auto gnss = input.gnss;
        gnss.latitude += 1;
        publish(dp::names::GNSS, gnss);
        auto recorder = input.recorder;
        ++recorder.interval;
        publish(dp::names::Recorder, recorder);
        auto work = input.work;
        work.pending = !work.pending;
        publish(dp::names::Work, work);
        auto wifi = input.wifi;
        ++wifi.ip[3];
        publish(dp::names::WiFi, wifi);
        auto battery = input.battery;
        battery.temperature = 68;
        publish(dp::names::Battery, battery);
        assert(changes == 1 && warning.active());
        ++battery.level_percent;
        publish(dp::names::Battery, battery);
        assert(changes == 2 && model->state().battery_percent == 48);
        publish(dp::names::Battery, battery);
        assert(changes == 2);
        recorder.reported_recording = true;
        publish(dp::names::Recorder, recorder);
        assert(changes == 3 && model->state().recording);
        lv_observer_delete(observer);

        /** @brief 更新领域 provider 后立即重建，首次显示只依赖 pull。 */
        warning.destroy();
        bar.destroy();
        input.battery.level_percent = 62;
        input.gnss.fix = hal::FixStatus::Float;
        input.gnss.satellites_used = 19;
        input.clock.hour = 8;
        lv_tick_inc(1000);
        DataProc_RunLoop();
        assert(bar.create(lv_layer_top()) && warning.create(lv_layer_top()) && warning.active());
        check_initial(bar, 62, i18n::text(i18n::TextId::PositionFloat));

        auto dialplate = std::make_unique<page::DialplateModel>();
        assert(dialplate->init() && dialplate->getPublishersNumber() == 5);
        check_subscription<dp::GNSS>(*dialplate);
        check_subscription<dp::Work>(*dialplate);
        check_subscription<dp::Recorder>(*dialplate);
        check_subscription<dp::Version>(*dialplate);
        /** @brief 第五个关系仅用于外观命令；精确计数排除 Status/System/Device 订阅。 */
        const DataProc::StatusBarPresentation presentation{false, {}};
        assert(dialplate->notify("StatusBar", &presentation, sizeof(presentation)) == DataNode::RES_OK);
        assert(dialplate->state().recording);
        changes = 0;
        observer = lv_subject_add_observer(dialplate->subject(), count_change, &changes);
        assert(observer && changes == 1);
        auto* root = lv_obj_create(lv_screen_active());
        page::DialplateView view;
        view.create(root);
        assert(view.bind(dialplate->subject()));
        assert(lv_ll_get_len(&dialplate->subject()->subs_ll) == 2);
        const auto* record = view.widgets().btn_cont.btn_rec;
        assert(lv_color_to_u32(lv_obj_get_style_border_color(record, LV_PART_MAIN))
               == lv_color_to_u32(page::style::kDanger));
        gnss = input.gnss;
        ++gnss.satellites_tracked;
        publish(dp::names::GNSS, gnss);
        assert(changes == 2);
        view.unbind();
        assert(lv_ll_get_len(&dialplate->subject()->subs_ll) == 1);
        ++gnss.satellites_tracked;
        publish(dp::names::GNSS, gnss);
        assert(changes == 3);
        assert(view.bind(dialplate->subject()));
        view.destroy();
        lv_obj_delete(root);
        assert(lv_ll_get_len(&dialplate->subject()->subs_ll) == 1);
        ++gnss.satellites_tracked;
        publish(dp::names::GNSS, gnss);
        assert(changes == 4);
        /** @brief Model 删除也会解除剩余 observer；随后发布不能回调已卸载页面。 */
        dialplate.reset();
        ++gnss.satellites_tracked;
        publish(dp::names::GNSS, gnss);
        assert(changes == 4 && !DataProc::Broker()->search("DialplateModel"));

        // SystemLoadingModel: publishers == 3 (Init, Version, StatusBar)
        auto loading = std::make_unique<page::SystemLoadingModel>();
        assert(loading->init() && loading->getPublishersNumber() == 3);
        check_subscription<dp::Init>(*loading);
        check_subscription<dp::Version>(*loading);
        assert(loading->state().simulated == input.board.capabilities.simulated);
        loading.reset();
        assert(!DataProc::Broker()->search("SystemLoadingModel"));

        // StartupModel: publishers == 4 (Battery, Version, Power, StatusBar)
        auto startup = std::make_unique<page::StartupModel>();
        assert(startup->init() && startup->getPublishersNumber() == 4);
        check_subscription<dp::Battery>(*startup);
        check_subscription<dp::Version>(*startup);
        check_subscription<dp::Power>(*startup);
        assert(startup->state().battery_percent == input.battery.level_percent);
        changes = 0;
        observer = lv_subject_add_observer(startup->subject(), count_change, &changes);
        assert(observer && changes == 1);
        publish(dp::names::Power, dp::PowerState{}); // Non-display field does not notify.
        assert(changes == 1);
        auto bat = input.battery;
        bat.level_percent = 99;
        publish(dp::names::Battery, bat);
        assert(changes == 2 && startup->state().battery_percent == 99);
        startup.reset();
        publish(dp::names::Battery, bat);
        assert(changes == 2 && !DataProc::Broker()->search("StartupModel"));

        // WorkSettingsModel: publishers == 2 (Work, StatusBar)
        auto work_model = std::make_unique<page::WorkSettingsModel>();
        assert(work_model->init() && work_model->getPublishersNumber() == 2);
        check_subscription<dp::Work>(*work_model);
        assert(work_model->state().work.mode == input.work.mode);
        changes = 0;
        observer = lv_subject_add_observer(work_model->subject(), count_change, &changes);
        assert(observer && changes == 1);
        auto w = input.work;
        w.radio_on = !w.radio_on;
        publish(dp::names::Work, w);
        assert(changes == 2);
        publish(dp::names::Work, w);
        assert(changes == 2); // Unchanged field does not notify.
        work_model.reset();
        publish(dp::names::Work, w);
        assert(changes == 2 && !DataProc::Broker()->search("WorkSettingsModel"));

        // RecordConfigModel: publishers == 3 (Recorder, Version, StatusBar)
        auto rec_model = std::make_unique<page::RecordConfigModel>();
        assert(rec_model->init() && rec_model->getPublishersNumber() == 3);
        check_subscription<dp::Recorder>(*rec_model);
        check_subscription<dp::Version>(*rec_model);
        assert(rec_model->state().recorder.recording == input.recorder.recording);
        changes = 0;
        observer = lv_subject_add_observer(rec_model->subject(), count_change, &changes);
        assert(observer && changes == 1);
        auto r = input.recorder;
        r.reported_recording = !r.reported_recording;
        publish(dp::names::Recorder, r);
        assert(changes == 1); // RecordConfig uses recording, not reported_recording!
        r.recording = !r.recording;
        publish(dp::names::Recorder, r);
        assert(changes == 2);
        rec_model.reset();
        publish(dp::names::Recorder, r);
        assert(changes == 2 && !DataProc::Broker()->search("RecordConfigModel"));

        // SystemInfosModel: publishers == 8 (Work, GNSS, WiFi, Battery, Recorder, Version, Init, StatusBar)
        auto sys_infos = std::make_unique<page::SystemInfosModel>();
        assert(sys_infos->init() && sys_infos->getPublishersNumber() == 8);
        check_subscription<dp::Work>(*sys_infos);
        check_subscription<dp::GNSS>(*sys_infos);
        check_subscription<dp::WiFi>(*sys_infos);
        check_subscription<dp::Battery>(*sys_infos);
        check_subscription<dp::Recorder>(*sys_infos);
        check_subscription<dp::Version>(*sys_infos);
        check_subscription<dp::Init>(*sys_infos);
        assert(sys_infos->state().gnss.satellites_used == input.gnss.satellites_used);
        changes = 0;
        observer = lv_subject_add_observer(sys_infos->subject(), count_change, &changes);
        assert(observer && changes == 1);
        sys_infos.reset();
        assert(!DataProc::Broker()->search("SystemInfosModel"));

        // SystemDashModel: publishers == 6 (Settings, WiFi, Version, Power, Clock, StatusBar)
        auto dash_model = std::make_unique<page::SystemDashModel>();
        assert(dash_model->init() && dash_model->getPublishersNumber() == 6);
        check_subscription<dp::Settings>(*dash_model);
        check_subscription<dp::WiFi>(*dash_model);
        check_subscription<dp::Version>(*dash_model);
        check_subscription<dp::Power>(*dash_model);
        check_subscription<dp::Clock>(*dash_model);
        changes = 0;
        observer = lv_subject_add_observer(dash_model->subject(), count_change, &changes);
        assert(observer && changes == 1);
        publish(dp::names::Clock, dp::ClockState{}); // Clock date is non-display in SystemDash.
        assert(changes == 1);
        auto wf = input.wifi;
        wf.on = !wf.on;
        publish(dp::names::WiFi, wf);
        assert(changes == 2);
        dash_model.reset();
        publish(dp::names::WiFi, wf);
        assert(changes == 2 && !DataProc::Broker()->search("SystemDashModel"));

        // StarMapModel: publishers == 3 (GNSS, Version, StatusBar)
        auto star_model = std::make_unique<page::StarMapModel>();
        assert(star_model->init() && star_model->getPublishersNumber() == 3);
        check_subscription<dp::GNSS>(*star_model);
        check_subscription<dp::Version>(*star_model);
        changes = 0;
        observer = lv_subject_add_observer(star_model->subject(), count_change, &changes);
        assert(observer && changes == 1);
        auto g = input.gnss;
        g.latitude += 1;
        publish(dp::names::GNSS, g);
        assert(changes == 1); // latitude is non-display in StarMap.
        g.constellations_valid = true;
        g.constellations[0] += 1;
        publish(dp::names::GNSS, g);
        assert(changes == 2);
        star_model.reset();
        publish(dp::names::GNSS, g);
        assert(changes == 2 && !DataProc::Broker()->search("StarMapModel"));

        // SaveConfigModel: publishers == 2 (Power, StatusBar)
        auto save_model = std::make_unique<page::SaveConfigModel>();
        assert(save_model->init() && save_model->getPublishersNumber() == 2);
        check_subscription<dp::Power>(*save_model);
        changes = 0;
        observer = lv_subject_add_observer(save_model->subject(), count_change, &changes);
        assert(observer && changes == 1);
        save_model.reset();
        assert(!DataProc::Broker()->search("SaveConfigModel"));

        warning.destroy();
        bar.destroy();
        publish(dp::names::Battery, input.battery);
    }
    assert(DataProc_Deinit());
    lv_display_delete(display);
    lv_deinit();
}
