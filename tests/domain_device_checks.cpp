#include "SystemState.h"
#include "hal_device_checks.h"
#include "mcu_config.h"

/** HC32/Board 取值和命令等价性；两款带屏产品各自运行同一组检查。 */
void
check_domain_devices() {
    hal_test::shapes();
    hal_test::equality();
    const PositionStatus_t fixes[] = {position_none, position_single, position_float, position_fix};
    const hal::FixStatus expected[] = {hal::FixStatus::None, hal::FixStatus::Single, hal::FixStatus::Float,
                                       hal::FixStatus::Fixed};
    for (unsigned sample = 0; sample < 12; ++sample) {
        State::Reset();
        auto& host = State::Host();
        auto& power = State::Power();
        host.position.coordinate_status = fixes[sample % 4];
        host.position.coordinate_lat = 31.25 + sample;
        host.position.coordinate_lon = 121.5 - sample;
        host.position.coordinate_alt = 12.5 + sample;
        host.position.satellite_number_used = sample;
        host.position.satellite_number_track = sample + 5;
        host.starMap.sequence = sample % 2;
        host.starMap.numberGPS = 1;
        host.starMap.numberBDS = 2;
        host.starMap.numberGLONASS = 3;
        host.starMap.numberGALILEO = 4;
        host.starMap.numberSBAS = 5;
        host.starMap.numberQZSS = 6;
        host.starMap.numberIRNSS = 7;
        host.work.work_mode = sample % 3 == 0 ? rover_mode : sample % 3 == 1 ? base_mode : autobase_mode;
        host.work.radio_status = sample % 2 ? On_Off_Status_ON : On_Off_Status_OFF;
        host.work.radio_protocol = sample % 2 ? 5 : 250;
        host.work.radio_channel = sample;
        host.work.NtripServer_status = sample % 3 == 1 ? On_Off_Status_ON : On_Off_Status_OFF;
        host.ntrip.NtripClient_status = sample % 3 == 2 ? On_Off_Status_ON : On_Off_Status_OFF;
        host.record.record_status = sample % 2 ? On_Off_Status_ON : On_Off_Status_OFF;
        host.record.record_type = sample % 2 ? 2 : 250;
        host.record.record_interval = sample % 2 ? 0x18 : 250;
        std::memset(host.record.record_name, 'R', sizeof(host.record.record_name));
        host.record.record_leftspace = 123.5f + sample;
        host.wifi.wifi_status = sample % 2 ? On_Off_Status_ON : On_Off_Status_OFF;
        host.wifi.wifi_mode = sample % 2;
        std::memset(host.wifi.wifi_ssid, 'S', sizeof(host.wifi.wifi_ssid));
        for (unsigned i = 0; i < 4; ++i) {
            host.wifi.wifi_ip[i] = sample + i;
        }
        power.batteryInfo.sample_valid = sample % 2;
        power.batteryInfo.Percent = 70 + sample;
        power.batteryInfo.Percent_f = 65.5f + sample;
        power.batteryInfo.Voltage_f = 7.1f + sample / 100.f;
        power.batteryInfo.Temp_f = -5.f + sample;
        power.batteryInfo.chargeStatus = sample % 3 == 0 ? notCharge : sample % 3 == 1 ? normalCharge : fastCharge;
        power.ExternalPower = sample % 2;
        power.panel_power_on = sample % 2;
        power.LinuxPowerOff = sample % 4 == 1;
        power.LowBatteryPowerOff = sample % 4 == 2;
        power.Force_ShutDown = sample % 4 == 3;
        power.ShutdownComplete = sample % 2;
        std::memset(State::Board().software_version, 'F', sizeof(systemInfo.board.software_version));
        std::memset(State::Board().hardware_version, 'H', sizeof(systemInfo.board.hardware_version));
        // 不同 pending 组合必须通过正式请求入口构造。
        if (sample % 2) {
            State::Request_Work(host.work);
            State::Request_Record(On_Off_Status_ON, 1, 0xff);
        }
        hal_test::equivalent();
        const auto gnss = hal_test::read<hal::GNSSState>(hal::names::GNSS);
        assert(gnss.fix == expected[sample % 4]);
        const auto board = hal_test::read<hal::BoardState>(hal::names::Board);
        assert(!board.capabilities.simulated && !board.capabilities.reboot_available);
        assert(board.capabilities.wifi_available == (PRODUCT_SHUTDOWN_WIFI_BUTTON != 0));
        assert(board.capabilities.star_map == (PRODUCT_PAGE_STAR_MAP != 0));
        assert(board.capabilities.record_settings == (PRODUCT_RECORD_SETTINGS != 0));
        const auto power_state = hal_test::read<hal::PowerState>(hal::names::Power);
        assert(power_state.powered_on == power.panel_power_on && power_state.standby == power.ShutdownComplete);
        assert(power_state.cause
               == (sample % 4 == 1   ? hal::PowerCause::Host
                   : sample % 4 == 2 ? hal::PowerCause::LowBattery
                   : sample % 4 == 3 ? hal::PowerCause::Forced
                                     : hal::PowerCause::User));
    }
    // 相同初值下，新旧命令必须产生同一个请求配置与 revision。
    for (hal::WorkTarget target : {hal::WorkTarget::Rover, hal::WorkTarget::Base, hal::WorkTarget::Ntrip}) {
        for (unsigned protocol = 0; protocol < hal::kRadioProtocolCount; ++protocol) {
            State::Reset();
            DataProc::DeviceRequest old;
            old.command = DataProc::DeviceCommand::ApplyWork;
            old.target = static_cast<DataProc::WorkTarget>(target);
            old.radio_protocol = protocol;
            old.radio_channel = 9;
            assert(system_backend::execute_device(old));
            const WorkConfig_t previous = State::Effective_Work();
            const auto revision = State::Work_Revision();
            State::Reset();
            assert(hal::write(hal_test::device(hal::names::Work), hal::WorkCommand{target, protocol, 9})
                   == DeviceObject::RES_OK);
            const WorkConfig_t current = State::Effective_Work();
            assert(std::memcmp(&previous, &current, sizeof(current)) == 0);
            assert(systemInfo.request.work_pending && State::Work_Revision() == revision);
            hal_test::equivalent();
        }
    }
    for (hal::RecordAction action : {hal::RecordAction::Start, hal::RecordAction::Stop}) {
        for (unsigned type = 0; type < hal::kRecordTypeCount; ++type) {
            for (unsigned interval = 0; interval < hal::kRecordIntervalCount; ++interval) {
                State::Reset();
                DataProc::DeviceRequest old;
                old.command = action == hal::RecordAction::Start ? DataProc::DeviceCommand::StartRecord
                                                                 : DataProc::DeviceCommand::StopRecord;
                old.record_type = type;
                old.record_interval = interval;
                assert(system_backend::execute_device(old));
                const RecordInfo_t previous = State::Effective_Record();
                const auto revision = State::Record_Revision();
                State::Reset();
                assert(hal::write(hal_test::device(hal::names::Recorder), hal::RecorderCommand{action, type, interval})
                       == DeviceObject::RES_OK);
                const RecordInfo_t current = State::Effective_Record();
                assert(std::memcmp(&previous, &current, sizeof(current)) == 0 && State::Record_Revision() == revision);
                assert(systemInfo.request.record_op && systemInfo.request.record_settings_pending);
                hal_test::equivalent();
            }
        }
    }
    State::Reset();
    for (bool on : {false, true}) {
        const bool old = system_backend::request_wifi(on);
        const auto previous = systemInfo.request.wifi_on;
        const int result = hal::write(hal_test::device(hal::names::WiFi), hal::WifiCommand{on});
        assert(old == (result == DeviceObject::RES_OK) && systemInfo.request.wifi_on == previous);
        assert(result == (PRODUCT_SHUTDOWN_WIFI_BUTTON ? DeviceObject::RES_OK : DeviceObject::RES_UNSUPPORT));
        hal_test::equivalent();
    }
    constexpr hal::InitStage stages[] = {hal::InitStage::Configuration, hal::InitStage::BatteryGauge,
                                         hal::InitStage::Charger, hal::InitStage::HostPower, hal::InitStage::HostLink};
    for (unsigned step = 0; step < hal::kInitializationStepCount; ++step) {
        hal::InitInfo info;
        info.step = step;
        assert(hal::ioctl(hal_test::device(hal::names::Init), hal::kInitializationInfo, info) == DeviceObject::RES_OK);
        assert(info.stage == stages[step] && std::strcmp(info.name, system_backend::initialization_name(step)) == 0);
        for (uint32_t elapsed : {0U, 30000U}) {
            hal::InitPoll poll;
            poll.step = step;
            poll.elapsed_ms = elapsed;
            char detail[96]{};
            const auto old = system_backend::poll_initialization_step(step, elapsed, detail, sizeof(detail));
            assert(hal::ioctl(hal_test::device(hal::names::Init), hal::kPollInitialization, poll)
                   == DeviceObject::RES_OK);
            assert(static_cast<int>(poll.status) == static_cast<int>(old)
                   && std::strcmp(poll.detail.data(), detail) == 0);
        }
    }
    // HC32 无重启能力；非法命令不得触发关机，开机 ioctl 的编码/尺寸必须严格匹配。
    State::Reset();
    assert(hal::write(hal_test::device(hal::names::Power), hal::PowerCommand::Reboot) == DeviceObject::RES_UNKNOWN);
    assert(!systemInfo.power.ShutdownReq);
    assert(hal::ioctl(hal_test::device(hal::names::Power), hal::kRequestPowerOn ^ 1U) == DeviceObject::RES_UNSUPPORT);
}
