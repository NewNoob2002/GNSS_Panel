#include "DataProc.h"
#include "DemoStatus.h"
#include "hal_device_checks.h"

int
main() {
    lv_init();
    hal_test::shapes();
    hal_test::equality();
    for (uint64_t elapsed : {uint64_t{0}, uint64_t{16000}, uint64_t{480000}, uint64_t{86400000}}) {
        DemoStatus::sample(elapsed);
        hal_test::equivalent();
        assert(hal_test::read<hal::ClockState>(hal::names::Clock).uptime_ms == elapsed);
        for (unsigned seed : {0U, 42U, 999U}) {
            DemoStatus::sample_random(elapsed, seed);
            hal_test::equivalent();
        }
    }
    const auto caps = hal_test::read<hal::BoardState>(hal::names::Board).capabilities;
    assert(caps.simulated && caps.reboot_available && !caps.wifi_available && caps.star_map && caps.record_settings);
    assert(hal_test::read<hal::GNSSState>(hal::names::GNSS).fix == hal::FixStatus::Single);
    for (hal::WorkTarget target : {hal::WorkTarget::Rover, hal::WorkTarget::Base, hal::WorkTarget::Ntrip}) {
        for (unsigned protocol = 0; protocol < hal::kRadioProtocolCount; ++protocol) {
            assert(hal::write(hal_test::device(hal::names::Work), hal::WorkCommand{target, protocol, 9})
                   == DeviceObject::RES_OK);
            const auto current = hal_test::read<hal::WorkState>(hal::names::Work);
            assert(current.mode == (target == hal::WorkTarget::Rover ? hal::WorkMode::Rover : hal::WorkMode::Base));
            assert(current.radio_on == (target != hal::WorkTarget::Ntrip));
            assert(current.ntrip == (target == hal::WorkTarget::Ntrip ? hal::NtripRole::Server : hal::NtripRole::None));
            if (current.radio_on) {
                assert(current.protocol == protocol && current.channel == 9);
            }
            hal_test::equivalent();
        }
    }
    for (hal::RecordAction action : {hal::RecordAction::Start, hal::RecordAction::Stop}) {
        for (unsigned type = 0; type < hal::kRecordTypeCount; ++type) {
            for (unsigned interval = 0; interval < hal::kRecordIntervalCount; ++interval) {
                assert(hal::write(hal_test::device(hal::names::Recorder), hal::RecorderCommand{action, type, interval})
                       == DeviceObject::RES_OK);
                const auto current = hal_test::read<hal::RecorderState>(hal::names::Recorder);
                assert(current.recording == (action == hal::RecordAction::Start)
                       && current.reported_recording == current.recording);
                if (current.recording) {
                    assert(current.type == type && current.interval == interval);
                }
                hal_test::equivalent();
            }
        }
    }
    for (bool on : {false, true}) {
        assert(hal::write(hal_test::device(hal::names::WiFi), hal::WifiCommand{on}) == DeviceObject::RES_UNSUPPORT);
        hal_test::equivalent();
    }
    constexpr hal::InitStage stages[] = {hal::InitStage::Configuration, hal::InitStage::Services, hal::InitStage::GNSS,
                                         hal::InitStage::Network, hal::InitStage::Finalization};
    for (unsigned step = 0; step < hal::kInitializationStepCount; ++step) {
        hal::InitInfo info;
        info.step = step;
        assert(hal::ioctl(hal_test::device(hal::names::Init), hal::kInitializationInfo, info) == DeviceObject::RES_OK);
        assert(info.stage == stages[step] && std::strcmp(info.name, system_backend::initialization_name(step)) == 0);
        for (uint32_t elapsed : {0U, 400U, 700U, 1200U, 30000U}) {
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
    assert(hal::ioctl(hal_test::device(hal::names::Power), hal::kRequestPowerOn) == DeviceObject::RES_OK);
    assert(hal::write(hal_test::device(hal::names::Power), hal::PowerCommand::Reboot) == DeviceObject::RES_OK);
    assert(hal::write(hal_test::device(hal::names::Power), hal::PowerCommand::Off) == DeviceObject::RES_OK);
    const auto power = hal_test::read<hal::PowerState>(hal::names::Power);
    assert(power.standby);
    lv_deinit();
}
