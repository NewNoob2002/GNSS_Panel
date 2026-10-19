#include "DialplateModel.h"

using namespace Page;

void
DialplateModel::Init() {
    account = new Account("DialplateModel", DataProc::Center(), 0, this);
    account->Subscribe("StatusBar");
    account->SetEventCallback(onEvent);
}

void
DialplateModel::Deinit() {
    if (account) {
        delete account;
        account = nullptr;
    }
}

int
DialplateModel::onEvent(Account* account, Account::EventParam_t* param) {
    (void)account;
    if (param->event != Account::EVENT_PUB_PUBLISH) {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    return Account::RES_OK;
}

void
DialplateModel::RecorderCommand(const RecCmd_t cmd) const {
    DataProc::StatusBar_Info_t statInfo;
    DATA_PROC_INIT_STRUCT(statInfo);
    statInfo.cmd = DataProc::STATUS_BAR_CMD_SET_LABEL_REC;

    switch (cmd) {
        case REC_START: statInfo.param.record_active = true; break;
        case REC_STOP: statInfo.param.record_active = false; break;
        default: break;
    }

    account->Notify("StatusBar", &statInfo, sizeof(statInfo));
}

void
DialplateModel::SetStatusBarStyle(const DataProc::StatusBar_Style_t style) const {
    DataProc::StatusBar_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.cmd = DataProc::STATUS_BAR_CMD_SET_STYLE;
    info.param.style = style;

    account->Notify("StatusBar", &info, sizeof(info));
}
