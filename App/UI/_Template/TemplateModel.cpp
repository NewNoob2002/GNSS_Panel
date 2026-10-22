#include "TemplateModel.h"

using namespace page;

bool TemplateModel::init() {
    account_ = std::make_unique<DataNode>("TemplateModel", DataProc::Broker(), this);
    return account_->isRegistered();
}

void TemplateModel::deinit() {
    account_.reset();
}
