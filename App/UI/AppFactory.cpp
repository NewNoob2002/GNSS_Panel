#include "AppFactory.h"
#include "Page.h"
#include "SystemService.h"

#include "UI/Dialplate/Dialplate.h"
#include "UI/RecordConfig/RecordConfig.h"
#include "UI/SaveConfig/SaveConfig.h"
#include "UI/StarMap/StarMap.h"
#include "UI/Startup/Startup.h"
#include "UI/SystemDash/SystemDash.h"
#include "UI/SystemInfos/SystemInfos.h"
#include "UI/SystemLoading/SystemLoading.h"
#include "UI/WorkSettings/WorkSettings.h"

namespace {
bool always_install() {
    return true;
}

bool has_star_map() {
    return SystemService::snapshot().star_map;
}

bool install(PageManager& manager, PageBase* page, const char* name) {
    if (manager.registerPage(page, name) != PageManager::RES_TYPE::OK) {
        delete page;
        return false;
    }
    page->onInstalled();
    return true;
}
} // namespace

bool AppFactory::installAll(PageManager& manager) {
#define PAGE_DEF(Name, InstallPredicate) \
    if (InstallPredicate() && !install(manager, new page::Name, "Pages/" #Name)) \
        return false;
#include "PageList.inc"
#undef PAGE_DEF
    return true;
}
