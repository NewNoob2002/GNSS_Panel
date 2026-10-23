#include "SystemLoadingModel.h"
#include <algorithm>
#include <cstdio>
#include "SystemBackend.h"
#include "Log.h"

using namespace page;

bool SystemLoadingModel::init() {
    /** @brief 订阅 Init 用于发送 Begin/Record 命令；订阅 Version 取 simulated 标志。 */
    if (!ready())
        return false;
    if (!dp::Init::subscribe(*this)) {
        APP_LOG_E("SystemLoadingModel", "Init subscribe failed");
        return false;
    }
    if (!subscribe("StatusBar")) {
        APP_LOG_E("SystemLoadingModel", "StatusBar subscribe failed");
        return false;
    }
    return load<dp::Version>();
}

bool SystemLoadingModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = isRegistered() && notify("StatusBar", &request, sizeof(request)) == RES_OK;
    if (!ok)
        APP_LOG_E("SystemLoadingModel", "StatusBar notification rejected");
    return ok;
}

bool SystemLoadingModel::begin_initialization() {
    const bool ok = dp::Init::notify(*this, {dp::InitOperation::Begin}) == RES_OK;
    if (!ok)
        APP_LOG_E("SystemLoading", "initialization report reset rejected");
    return ok;
}

DataProc::InitializationStatus SystemLoadingModel::poll_step(unsigned step, uint32_t elapsed_ms) {
    if (!isRegistered() || step >= DataProc::kInitializationStepCount) {
        APP_LOG_E("SystemLoading", "invalid/uninitialized step %u", step);
        return DataProc::InitializationStatus::Failed;
    }
    const char* name = system_backend::initialization_name(step);
    const char* tag = state_.simulated ? " [DEMO]" : "";
    if (elapsed_ms == 0)
        APP_LOG_I("SystemLoading", "step %u/5 %s: begin%s", step + 1, name, tag);

    DataProc::InitializationResult result{};
    auto& detail = result.detail;
    auto status = system_backend::poll_initialization_step(step, elapsed_ms, detail.data(), detail.size());
    if (status == DataProc::InitializationStatus::Pending && elapsed_ms >= kStepLimitMs) {
        std::snprintf(detail.data(), detail.size(), "no result within %lu ms",
                      static_cast<unsigned long>(kStepLimitMs));
        status = DataProc::InitializationStatus::Failed;
    }
    if (status == DataProc::InitializationStatus::Pending)
        return status;
    if (status != DataProc::InitializationStatus::Ok)
        status = DataProc::InitializationStatus::Failed;
    detail.back() = '\0';
    result.status = status;
    const bool success = status == DataProc::InitializationStatus::Ok;
    app_log_write(success ? APP_LOG_INFO : APP_LOG_WARN, "SystemLoading",
                  "step %u/5 %s: %s%s after %lu ms (%s)", step + 1, name,
                  success ? "OK" : "FAILED", tag, static_cast<unsigned long>(elapsed_ms), detail.data());
    if (dp::Init::notify(*this, {dp::InitOperation::Record, step, result}) != RES_OK)
        APP_LOG_E("SystemLoading", "step %u report rejected", step + 1);
    return status;
}

int SystemLoadingModel::onEvent(EventParam_t* event) {
    /** @brief 只订阅 Version；PUBLISH 来了直接更新字段，不 changed()（无 View 绑定）。 */
    dp::Version::State value;
    const int result = dp::Version::receive(event, value);
    if (result == RES_OK)
        apply(value);
    return result;
}

void SystemLoadingModel::apply(const dp::Version::State& value) {
    state_.simulated = value.capabilities.simulated;
    // 无 View 订阅，不需要 changed()；Page 在 onViewLoad 时读一次 state_.simulated。
}
