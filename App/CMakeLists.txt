# 共享应用只消费显式的 L0 选项与 LVGL 目标。
include(${CMAKE_CURRENT_LIST_DIR}/oem.cmake)
if(NOT TARGET proj::options)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/options.cmake)
endif()

add_subdirectory(Utils/Spring)

# 日志接口头单独成目标，平台可只取接口而自行提供实现（如 HC32 release 的空实现）。
add_library(app_log_headers INTERFACE)
add_library(app::log_headers ALIAS app_log_headers)
target_include_directories(app_log_headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/Utils/Log")
# Release 类配置不保留逐对象/逐帧 DEBUG 实参；未指定构建类型按开发构建保留全部级别。默认运行时级别与编译下限一致。
target_compile_definitions(app_log_headers INTERFACE
    APP_LOG_COMPILE_LEVEL=$<IF:$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>,1,0>)
add_library(app_log STATIC Utils/Log/Log.cpp)
add_subdirectory(../Frameworks/DataBroker frameworks/databroker)
add_subdirectory(../Frameworks/DeviceManager frameworks/devicemanager)
add_subdirectory(../Frameworks/ResourceManager frameworks/resourcemanager)
# 保留原静态归档及对象顺序，由各组件维护自己的源文件。
add_library(app_framework STATIC)
target_link_libraries(app_framework PUBLIC fw::databroker fw::devicemanager fw::resourcemanager)
add_library(app_i18n STATIC
    Service/i18n/I18n.cpp Service/i18n/I18n_en.cpp Service/i18n/I18n_ru.cpp)
add_library(app_core STATIC
    Service/DataProc/DataProc.cpp
    Service/DataProc/DP_Power.cpp
    Service/DataProc/DP_Button.cpp
    Service/DataProc/DP_PageNavi.cpp
    Service/DataProc/DP_Battery.cpp
    Service/DataProc/DP_GNSS.cpp
    Service/DataProc/DP_Clock.cpp
    Service/DataProc/DP_Work.cpp
    Service/DataProc/DP_Recorder.cpp
    Service/DataProc/DP_WiFi.cpp
    Service/DataProc/DP_Version.cpp
    Service/DataProc/DP_Init.cpp
    Service/DataProc/DP_Settings.cpp
    Service/HAL/PowerObject.cpp
    Utils/ButtonGesture/ButtonGesture.cpp
    Utils/FixedFormat/FixedFormat.cpp)
foreach(target app_log app_framework app_i18n app_core)
    target_link_libraries(${target} PUBLIC proj::options)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
endforeach()
add_library(app::log ALIAS app_log)
add_library(app::framework ALIAS app_framework)
add_library(app::i18n ALIAS app_i18n)
add_library(app::core ALIAS app_core)
target_link_libraries(app_log PUBLIC app::log_headers)
target_include_directories(app_i18n PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Service/i18n"
    "${APP_OEM_DIR}/generated")
target_include_directories(app_core PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/Service"
    "${CMAKE_CURRENT_SOURCE_DIR}/Service/DataProc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Service/HAL"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils/FixedFormat"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils/ButtonGesture")
target_link_libraries(app_framework PRIVATE app::log)
target_link_libraries(app_core PUBLIC app::framework app::i18n app::log)
# 平台继承核心设备接口，必须与核心保持一致的 RTTI 设置。
target_compile_options(app_core PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

# 无图形构建只生成核心目标。
if(NOT TARGET lvgl::lvgl)
    return()
endif()
add_subdirectory(../Frameworks/PageManager frameworks/pagemanager)

file(GLOB APP_RESOURCES CONFIGURE_DEPENDS "UI/Resource/font/*.c" "UI/Resource/img/*.c")
add_library(app_dialplate STATIC
    UI/Common/FocusHighlight.cpp
    UI/Common/LogoReveal.cpp
    UI/Dialplate/DialplateView.cpp
    UI/Resource/ResourcePool.cpp
    Utils/lv_anim_label/numberFlow.cpp
    Utils/lv_anim_label/numberFlow_clock.cpp
    Utils/lv_ext/lv_anim_timeline_wrapper.c
    ${APP_RESOURCES}
    "${APP_OEM_DIR}/generated/oem_logo.c")
target_include_directories(app_dialplate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(app::dialplate ALIAS app_dialplate)
target_link_libraries(app_dialplate PUBLIC lvgl::lvgl app::i18n proj::options fw::resourcemanager app::core app::lv_spring)
target_compile_options(app_dialplate PRIVATE -Wall -Wextra)

add_library(app_ui STATIC
    App.cpp
    UI/AppFactory.cpp
    UI/AppNavigator.cpp
    UI/InputRouter.cpp
    UI/Dialplate/DialplateModel.cpp
    UI/WorkSettings/WorkSettingsModel.cpp
    UI/RecordConfig/RecordConfigModel.cpp
    UI/SystemInfos/SystemInfosModel.cpp
    UI/SystemDash/SystemDashModel.cpp
    UI/StarMap/StarMapModel.cpp
    UI/Startup/StartupModel.cpp
    UI/SystemLoading/SystemLoadingModel.cpp
    UI/SaveConfig/SaveConfigModel.cpp
    UI/StatusBar/StatusBar.cpp
    UI/StatusBar/StatusBarModel.cpp
    UI/BatteryWarning/BatteryWarning.cpp
    UI/BatteryWarning/BatteryWarningModel.cpp
    UI/Dialplate/Dialplate.cpp
    UI/WorkSettings/WorkSettings.cpp
    UI/WorkSettings/WorkSettingsView.cpp
    UI/RecordConfig/RecordConfig.cpp
    UI/RecordConfig/RecordConfigView.cpp
    UI/SystemInfos/SystemInfos.cpp
    UI/SystemInfos/SystemInfosView.cpp
    UI/SystemDash/SystemDash.cpp
    UI/SystemDash/SystemDashView.cpp
    UI/StarMap/StarMap.cpp
    UI/StarMap/StarMapView.cpp
    UI/Startup/Startup.cpp
    UI/Startup/StartupView.cpp
    UI/SystemLoading/SystemLoading.cpp
    UI/SystemLoading/SystemLoadingView.cpp
    UI/SaveConfig/SaveConfig.cpp
    UI/SaveConfig/SaveConfigView.cpp)
target_include_directories(app_ui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/UI/Common"
    "${CMAKE_CURRENT_SOURCE_DIR}/Utils/ButtonGesture")
add_library(app::ui ALIAS app_ui)
target_link_libraries(app_ui PUBLIC app::core fw::pagemanager app::dialplate app::lv_spring)
target_compile_options(app_ui PRIVATE -Wall -Wextra -Werror)
