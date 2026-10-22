#include "SystemLoadingModel.h"
#include <algorithm>
#include <cstdio>
#include "ModelUtils.h"
#include "SystemBackend.h"
#include "SystemService.h"
#include "Log.h"

using namespace page;

bool SystemLoadingModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<DataNode>("SystemLoadingModel", DataProc::Broker(), this);
    account_->setEventCallback(on_event);
    if (!account_->isRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("SystemLoadingModel", "DataNode initialization failed");
        return false;
    }
    return true;
}
void SystemLoadingModel::deinit() {
    account_.reset();
}
bool SystemLoadingModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->notify("StatusBar", &request, sizeof(request)) == DataNode::RES_OK;
    if (!ok)
        APP_LOG_E("SystemLoadingModel", "StatusBar notification rejected");
    return ok;
}
bool SystemLoadingModel::begin_initialization() const {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::BeginInitialization;
    const bool ok = account_ && account_->notify("System", &request, sizeof(request)) == DataNode::RES_OK;
    if (!ok)
        APP_LOG_E("SystemLoading", "initialization report reset rejected");
    return ok;
}
DataProc::InitializationStatus SystemLoadingModel::poll_step(unsigned step, std::uint32_t elapsed_ms) const {
    if (!account_ || step >= DataProc::kInitializationStepCount) {
        APP_LOG_E("SystemLoading", "invalid/uninitialized step %u", step);
        return DataProc::InitializationStatus::Failed;
    }
    const char* name = system_backend::initialization_name(step);
    const char* tag = state_.simulated ? " [DEMO]" : "";
    if (elapsed_ms == 0)
        APP_LOG_I("SystemLoading", "step %u/5 %s: begin%s", step + 1, name, tag);
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::RecordInitialization;
    request.initialization_step = step;
    auto& detail = request.initialization.detail;
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
    request.initialization.status = status;
    const bool success = status == DataProc::InitializationStatus::Ok;
    app_log_write(success ? APP_LOG_INFO : APP_LOG_WARN, "SystemLoading", "step %u/5 %s: %s%s after %lu ms (%s)",
                  step + 1, name, success ? "OK" : "FAILED", tag, static_cast<unsigned long>(elapsed_ms),
                  detail.data());
    if (account_->notify("System", &request, sizeof(request)) != DataNode::RES_OK)
        APP_LOG_E("SystemLoading", "step %u report rejected", step + 1);
    return status;
}

int SystemLoadingModel::on_event(DataNode* account, DataNode::EventParam_t* event) {
    auto* self = static_cast<SystemLoadingModel*>(account->getUserData());
    return model_utils::receive(event, self->status_, self->state_);
}
