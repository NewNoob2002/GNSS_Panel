# Shared application: portable framework/core; UI is enabled by the parent-provided p4_lvgl target.
if(NOT TARGET p4_build_options)
    add_library(p4_build_options INTERFACE)
endif()

add_library(p4_log STATIC Utils/Log/Log.cpp)
add_library(p4_data_center STATIC
    Frameworks/DataCenter/Account.cpp
    Frameworks/DataCenter/DataCenter.cpp
    Frameworks/DataCenter/DataTimer.cpp
    Frameworks/DataCenter/PingPongBuffer/PingPongBuffer.c)
add_library(p4_i18n STATIC
    Utils/I18n/I18n.cpp Utils/I18n/I18n_en.cpp Utils/I18n/I18n_ru.cpp)
add_library(p4_core STATIC
    Common/DataProc/DataProc.cpp
    Common/DataProc/DP_Status.cpp
    Common/DataProc/DP_Device.cpp
    Common/SystemService.cpp)
foreach(target p4_log p4_data_center p4_i18n p4_core)
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} PUBLIC p4_build_options)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
endforeach()
target_link_libraries(p4_data_center PRIVATE p4_log)
target_link_libraries(p4_core PUBLIC p4_data_center p4_i18n p4_log)

# A headless parent builds only the targets above; no LVGL headers or library are required.
if(NOT TARGET p4_lvgl)
    return()
endif()
add_library(p4_page_manager STATIC
    Utils/PageManager/PageBase.cpp
    Utils/PageManager/PageManager_Base.cpp
    Utils/PageManager/PageManager_Router.cpp
    Utils/PageManager/PageManager_State.cpp
    Utils/PageManager/PageManager_Anim.cpp
    Utils/PageManager/PageManager_Drag.cpp)
target_include_directories(p4_page_manager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(p4_page_manager PUBLIC p4_lvgl p4_build_options PRIVATE p4_log)
target_compile_options(p4_page_manager PRIVATE -Wall -Wextra)

file(GLOB P4_RESOURCES CONFIGURE_DEPENDS "Resource/font/*.c" "Resource/img/*.c" "Resource/symbol/*.c")
add_library(p4_dialplate STATIC
    Pages/Dialplate/DialplateView.cpp
    Resource/ResourcePool.cpp
    Utils/ResourceManager/ResourceManager.cpp
    Utils/lv_anim_label/numberFlow.cpp
    Utils/lv_anim_label/numberFlow_clock.cpp
    Utils/lv_ext/lv_anim_timeline_wrapper.c
    ${P4_RESOURCES})
target_include_directories(p4_dialplate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(p4_dialplate PUBLIC p4_lvgl p4_i18n p4_build_options)
target_compile_options(p4_dialplate PRIVATE -Wall -Wextra)

add_library(p4_app STATIC
    ButtonGesture.cpp
    P4App.cpp
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
target_include_directories(p4_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(p4_app PUBLIC p4_core p4_page_manager p4_dialplate)
target_compile_options(p4_app PRIVATE -Wall -Wextra -Werror)
