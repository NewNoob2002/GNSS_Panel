#include "AppFactory.h"
#include "PageId.h"
#include "Log.h"
#include <cstring>
#include "Page.h"
#include "Def/DP_Version.h"

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
    dp::Version::State version;
    return dp::Version::pull(*DataProc::Broker()->mainNode(), version) == DataNode::RES_OK
        && version.capabilities.star_map;
}

bool install(PageManager& manager, PageBase* page, const char* name) {
    if (manager.registerPage(page, name) != PageManager::RES_TYPE::OK) {
        APP_LOG_E("PageManager", "page installation failed: %s", name);
        delete page;
        return false;
    }
    page->onInstalled();
    return true;
}
} // namespace

const char* page::name(PageId id) {
    static constexpr const char* names[] = {
#define PAGE_DEF(Name, InstallPredicate) "Pages/" #Name,
#include "PageList.inc"
#undef PAGE_DEF
    };
    return id < PageId::Count ? names[static_cast<uint16_t>(id)] : nullptr;
}

page::PageId page::id(const char* route) {
    if (route) {
        for (uint16_t i = 0; i < static_cast<uint16_t>(PageId::Count); ++i) {
            const auto id = static_cast<PageId>(i);
            if (std::strcmp(route, name(id)) == 0)
                return id;
        }
    }
    return PageId::Invalid;
}

bool AppFactory::installAll(PageManager& manager) {
    [[maybe_unused]] unsigned installed = 0;
#define PAGE_DEF(Name, InstallPredicate) \
    if (InstallPredicate()) { \
        if (!install(manager, new page::Name, page::name(page::PageId::Name))) \
            return false; \
        ++installed; \
    }
#include "PageList.inc"
#undef PAGE_DEF
    APP_LOG_I("PageManager", "pages installed: %u", installed);
    return true;
}
