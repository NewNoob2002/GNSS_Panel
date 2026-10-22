# Shared application: portable framework/core; UI is enabled by the parent-provided app_lvgl target.
if(NOT TARGET app_build_options)
    add_library(app_build_options INTERFACE)
endif()

add_library(app_log STATIC Utils/Log/Log.cpp)
add_library(app_framework STATIC
    ../Frameworks/DataBroker/DataNode.cpp
    ../Frameworks/DataBroker/DataBroker.cpp
    ../Frameworks/DataBroker/DataTimer.cpp
    ../Frameworks/DeviceManager/DeviceManager.cpp
    ../Frameworks/ResourceManager/ResourceManager.h)
add_library(app_i18n STATIC
    Utils/I18n/I18n.cpp Utils/I18n/I18n_en.cpp Utils/I18n/I18n_ru.cpp)
add_library(app_core STATIC
    Common/DataProc/DataProc.cpp
    Common/DataProc/DP_Status.cpp
    Common/DataProc/DP_Device.cpp
    Common/SystemService.cpp
    Common/SystemBackend.cpp)
foreach(target app_log app_framework app_i18n app_core)
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
    target_link_libraries(${target} PUBLIC app_build_options)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
endforeach()
target_link_libraries(app_framework PRIVATE app_log)
target_link_libraries(app_core PUBLIC app_framework app_i18n app_log)
# 平台继承核心设备接口，必须与核心保持一致的 RTTI 设置。
target_compile_options(app_core PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

# A headless parent builds only the targets above; no LVGL headers or library are required.
if(NOT TARGET app_lvgl)
    return()
endif()
add_library(app_page_manager STATIC
    Utils/PageManager/PageBase.cpp
    Utils/PageManager/PageManager_Base.cpp
    Utils/PageManager/PageManager_Router.cpp
    Utils/PageManager/PageManager_State.cpp
    Utils/PageManager/PageManager_Anim.cpp
    Utils/PageManager/PageManager_Drag.cpp)
target_include_directories(app_page_manager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_page_manager PUBLIC app_lvgl app_build_options PRIVATE app_log)
target_compile_options(app_page_manager PRIVATE -Wall -Wextra)
# PageBase 与 app_ui 中的派生页面必须使用一致的 RTTI 布局及检查策略。
target_compile_options(app_page_manager PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

file(GLOB APP_RESOURCES CONFIGURE_DEPENDS "Resource/font/*.c" "Resource/img/*.c" "Resource/symbol/*.c")
add_library(app_dialplate STATIC
    Pages/Dialplate/DialplateView.cpp
    Resource/ResourcePool.cpp
    Utils/lv_anim_label/numberFlow.cpp
    Utils/lv_anim_label/numberFlow_clock.cpp
    Utils/lv_ext/lv_anim_timeline_wrapper.c
    ${APP_RESOURCES})
target_include_directories(app_dialplate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_dialplate PUBLIC app_lvgl app_i18n app_build_options)
target_compile_options(app_dialplate PRIVATE -Wall -Wextra)

add_library(app_ui STATIC
    ButtonGesture.cpp
    App.cpp
    Pages/Dialplate/DialplateModel.cpp
    Pages/WorkSettings/WorkSettingsModel.cpp
    Pages/RecordConfig/RecordConfigModel.cpp
    Pages/SystemInfos/SystemInfosModel.cpp
    Pages/SystemDash/SystemDashModel.cpp
    Pages/StarMap/StarMapModel.cpp
    Pages/Startup/StartupModel.cpp
    Pages/SystemLoading/SystemLoadingModel.cpp
    Pages/SaveConfig/SaveConfigModel.cpp
    Pages/StatusBar/StatusBar.cpp
    Pages/BatteryWarning/BatteryWarning.cpp
    Pages/Dialplate/Dialplate.cpp
    Pages/WorkSettings/WorkSettings.cpp
    Pages/WorkSettings/WorkSettingsView.cpp
    Pages/RecordConfig/RecordConfig.cpp
    Pages/RecordConfig/RecordConfigView.cpp
    Pages/SystemInfos/SystemInfos.cpp
    Pages/SystemInfos/SystemInfosView.cpp
    Pages/SystemDash/SystemDash.cpp
    Pages/SystemDash/SystemDashView.cpp
    Pages/StarMap/StarMap.cpp
    Pages/StarMap/StarMapView.cpp
    Pages/Startup/Startup.cpp
    Pages/Startup/StartupView.cpp
    Pages/SystemLoading/SystemLoading.cpp
    Pages/SystemLoading/SystemLoadingView.cpp
    Pages/SaveConfig/SaveConfig.cpp
    Pages/SaveConfig/SaveConfigView.cpp)
target_include_directories(app_ui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_ui PUBLIC app_core app_page_manager app_dialplate)
target_compile_options(app_ui PRIVATE -Wall -Wextra -Werror)
