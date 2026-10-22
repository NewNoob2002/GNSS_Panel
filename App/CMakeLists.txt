# 共享应用只消费显式的 L0 选项与 LVGL 目标。
if(NOT TARGET proj::options)
    include(${CMAKE_CURRENT_LIST_DIR}/../cmake/options.cmake)
endif()

add_library(app_log STATIC Utils/Log/Log.cpp)
add_subdirectory(../Frameworks/DataBroker frameworks/databroker)
add_subdirectory(../Frameworks/DeviceManager frameworks/devicemanager)
add_subdirectory(../Frameworks/ResourceManager frameworks/resourcemanager)
# 保留原静态归档及对象顺序，由各组件维护自己的源文件。
add_library(app_framework STATIC)
target_link_libraries(app_framework PUBLIC fw::databroker fw::devicemanager fw::resourcemanager)
add_library(app_i18n STATIC
    Utils/I18n/I18n.cpp Utils/I18n/I18n_en.cpp Utils/I18n/I18n_ru.cpp)
add_library(app_core STATIC
    Common/DataProc/DataProc.cpp
    Common/DataProc/DP_Status.cpp
    Common/DataProc/DP_Device.cpp
    Common/SystemService.cpp
    Common/SystemBackend.cpp)
foreach(target app_log app_framework app_i18n app_core)
    target_link_libraries(${target} PUBLIC proj::options)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
endforeach()
add_library(app::log ALIAS app_log)
add_library(app::framework ALIAS app_framework)
add_library(app::i18n ALIAS app_i18n)
add_library(app::core ALIAS app_core)
target_include_directories(app_log PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Utils/Log")
target_include_directories(app_i18n PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Utils/I18n")
target_include_directories(app_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Common")
target_link_libraries(app_framework PRIVATE app::log)
target_link_libraries(app_core PUBLIC app::framework app::i18n app::log)
# 平台继承核心设备接口，必须与核心保持一致的 RTTI 设置。
target_compile_options(app_core PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

# 无图形构建只生成核心目标。
if(NOT TARGET lvgl::lvgl)
    return()
endif()
add_subdirectory(../Frameworks/PageManager frameworks/pagemanager)

file(GLOB APP_RESOURCES CONFIGURE_DEPENDS "Resource/font/*.c" "Resource/img/*.c" "Resource/symbol/*.c")
add_library(app_dialplate STATIC
    Pages/Dialplate/DialplateView.cpp
    Resource/ResourcePool.cpp
    Utils/lv_anim_label/numberFlow.cpp
    Utils/lv_anim_label/numberFlow_clock.cpp
    Utils/lv_ext/lv_anim_timeline_wrapper.c
    ${APP_RESOURCES})
target_include_directories(app_dialplate PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
add_library(app::dialplate ALIAS app_dialplate)
target_link_libraries(app_dialplate PUBLIC lvgl::lvgl app::i18n proj::options fw::resourcemanager app::core)
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
add_library(app::ui ALIAS app_ui)
target_link_libraries(app_ui PUBLIC app::core fw::pagemanager app::dialplate)
target_compile_options(app_ui PRIVATE -Wall -Wextra -Werror)
