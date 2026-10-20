#include "SystemLoadingModel.h"
#include <algorithm>
#include <cstdio>
#include "Common/ModelUtils.h"
#include "Common/SystemBackend.h"
#include "Common/SystemService.h"
#include "Utils/Log/Log.h"

using namespace page;

bool SystemLoadingModel::init() {
    if (account_)
        return true;
    account_ = std::make_unique<Account>("SystemLoadingModel", DataProc::Center(), 0, this);
    account_->SetEventCallback(on_event);
    if (!account_->IsRegistered() || !model_utils::subscribe(*account_, status_, state_)) {
        account_.reset();
        APP_LOG_E("SystemLoadingModel", "Account initialization failed");
        return false;
    }
    return true;
}
void SystemLoadingModel::deinit() {
    account_.reset();
}
bool SystemLoadingModel::set_status_bar(bool visible, DataProc::StatusBarStyle style) const {
    const DataProc::StatusBarPresentation request{visible, style};
    const bool ok = account_ && account_->Notify("StatusBar", &request, sizeof(request)) == Account::RES_OK;
    if (!ok)
        APP_LOG_E("SystemLoadingModel", "StatusBar notification rejected");
    return ok;
}
bool SystemLoadingModel::begin_initialization() const {
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::BeginInitialization;
    const bool ok = account_ && account_->Notify("System", &request, sizeof(request)) == Account::RES_OK;
    if (!ok)
        APP_LOG_E("SystemLoading", "initialization report reset rejected");
    return ok;
}
bool SystemLoadingModel::initialize_step(unsigned step) const {
    if (!account_ || step >= DataProc::kInitializationStepCount) {
        APP_LOG_E("SystemLoading", "invalid/uninitialized step %u", step);
        return false;
    }
    const char* name = system_backend::initialization_name(step);
    const char* tag = state_.simulated ? " [DEMO]" : "";
    APP_LOG_I("SystemLoading", "step %u/5 %s: begin%s", step + 1, name, tag);
    DataProc::SystemRequest request{};
    request.command = DataProc::SystemCommand::RecordInitialization;
    request.initialization_step = step;
    const bool success = system_backend::initialize_step(step, request.initialization.detail.data(),
                                                         request.initialization.detail.size());
    request.initialization.detail.back() = '\0';
    request.initialization.status =
        success ? DataProc::InitializationStatus::Ok : DataProc::InitializationStatus::Failed;
    app_log_write(success ? APP_LOG_INFO : APP_LOG_WARN, "SystemLoading", "step %u/5 %s: %s%s (%s)", step + 1, name,
                  success ? "OK" : "FAILED", tag, request.initialization.detail.data());
    if (account_->Notify("System", &request, sizeof(request)) != Account::RES_OK) {
        APP_LOG_E("SystemLoading", "step %u report rejected", step + 1);
        return false;
    }
    return success;
}

int SystemLoadingModel::on_event(Account* account, Account::EventParam_t* event) {
    auto* self = static_cast<SystemLoadingModel*>(account->UserData);
    return model_utils::receive(event, self->status_, self->state_);
}
