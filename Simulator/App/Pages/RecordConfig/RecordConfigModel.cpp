//
// Created by gtc on 2026/5/11.
//

#include "RecordConfigModel.h"

using namespace Page;

void
RecordConfigModel::Init() {
    account = new Account("RecordConfigModel", DataProc::Center(), 0, this);
    account->Subscribe("StatusBar");
    account->SetEventCallback(onEvent);
}

void
RecordConfigModel::Deinit() {
    if (account) {
        delete account;
        account = nullptr;
    }
}

int
RecordConfigModel::onEvent(Account* account, Account::EventParam_t* param) {
    (void)account;
    if (param->event != Account::EVENT_PUB_PUBLISH) {
        return Account::RES_UNSUPPORTED_REQUEST;
    }

    return Account::RES_OK;
}

void
RecordConfigModel::RecorderCommand(const RecCmd_t cmd) const {
    DataProc::StatusBar_Info_t statusInfo;
    DATA_PROC_INIT_STRUCT(statusInfo);
    statusInfo.cmd = DataProc::STATUS_BAR_CMD_SET_LABEL_REC;

    switch (cmd) {
        case REC_START: statusInfo.param.record_active = true; break;
        case REC_STOP: statusInfo.param.record_active = false; break;
        default: break;
    }

    account->Notify("StatusBar", &statusInfo, sizeof(statusInfo));
}

void
RecordConfigModel::SetStatusBarStyle(const DataProc::StatusBar_Style_t style) const {
    DataProc::StatusBar_Info_t info;
    DATA_PROC_INIT_STRUCT(info);

    info.cmd = DataProc::STATUS_BAR_CMD_SET_STYLE;
    info.param.style = style;

    account->Notify("StatusBar", &info, sizeof(info));
}
