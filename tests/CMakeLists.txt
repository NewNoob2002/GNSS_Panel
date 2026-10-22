cmake_minimum_required(VERSION 3.20)
project(hc32_regressions LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()
set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(HC32 ${ROOT}/Vendor/HC32)
# 系统状态分域（所有使用 systemInfo 的测试都链接它）
set(STATE_SOURCES ${HC32}/Platform/State/SystemState.cpp)
set(STATE_INCLUDE ${HC32}/Platform/State)
set(PARSER_SOURCES
    ${HC32}/Libraries/Parser/src/SparkFun_Extensible_Message_Parser.cpp
    ${HC32}/Libraries/Parser/src/Parse_CUSTOM.cpp)
# 传输层、电源与 X1Pro 协议表（带屏产品基线）
add_executable(firmware_tests firmware_tests.cpp ${STATE_SOURCES}
    ${HC32}/Comm/slave_i2c.cpp ${HC32}/Comm/panel_protocol.cpp
    ${HC32}/Display/display_protocol.cpp
    ${PARSER_SOURCES}
    ${HC32}/Platform/Peripherals/I2cMaster.cpp ${HC32}/Platform/Config/board_peripherals.cpp ${HC32}/Platform/HAL/HAL_Power.cpp ${HC32}/Platform/HAL/HAL_Battery.cpp ${HC32}/Platform/HAL/Charger.cpp
    ${HC32}/Libraries/Bq40z50/bq40z50.cpp ${HC32}/Libraries/Mp2762a/mp2762a.cpp)
target_include_directories(firmware_tests PRIVATE fakes ${STATE_INCLUDE} ${HC32}/Comm ${HC32}/Display
    ${HC32}/Products/X1Pro ${HC32}/Platform/Config
    ${HC32}/Platform/Peripherals ${HC32}/Platform/System ${HC32}/Platform/HAL ${HC32}/Libraries/Parser/include
    ${HC32}/Libraries/Bq40z50 ${HC32}/Libraries/Mp2762a)

# 各产品协议表：同一测试源分别按产品配置编译
add_executable(x1_protocol_tests product_protocol_tests.cpp ${STATE_SOURCES}
    ${HC32}/Comm/panel_protocol.cpp ${HC32}/Products/X1/x1_protocol.cpp ${PARSER_SOURCES})
target_include_directories(x1_protocol_tests PRIVATE fakes ${STATE_INCLUDE} ${HC32}/Comm ${HC32}/Products/X1
    ${HC32}/Platform/Config ${HC32}/Libraries/Parser/include)
add_executable(xbase_protocol_tests product_protocol_tests.cpp ${STATE_SOURCES}
    ${HC32}/Comm/panel_protocol.cpp ${HC32}/Display/display_protocol.cpp ${PARSER_SOURCES})
target_include_directories(xbase_protocol_tests PRIVATE fakes ${STATE_INCLUDE} ${HC32}/Comm ${HC32}/Display
    ${HC32}/Products/XBase ${HC32}/Platform/Config ${HC32}/Libraries/Parser/include)
target_compile_options(firmware_tests PRIVATE -include ${CMAKE_CURRENT_LIST_DIR}/fakes/HAL.h)
target_compile_definitions(firmware_tests PRIVATE __CORE_DEBUG __HAL_H)
add_executable(clock_tests clock_tests.c)
target_include_directories(clock_tests PRIVATE time_fakes)
add_executable(heap_tests heap_tests.c ${HC32}/Platform/System/Syscalls_GCC.c)
target_compile_definitions(heap_tests PRIVATE _DEFAULT_SOURCE)
# 同一 HAL 电源回归也按 X1/XBase 开关编译，验证无屏按键路径与两种带屏配置。
foreach(product X1 XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_power_tests firmware_tests.cpp ${STATE_SOURCES}
        ${HC32}/Platform/HAL/HAL_Power.cpp ${HC32}/Platform/HAL/Charger.cpp
        ${HC32}/Platform/HAL/HAL_Battery.cpp ${HC32}/Platform/Peripherals/I2cMaster.cpp ${HC32}/Platform/Config/board_peripherals.cpp
        ${HC32}/Libraries/Bq40z50/bq40z50.cpp ${HC32}/Libraries/Mp2762a/mp2762a.cpp)
    target_include_directories(${name}_power_tests PRIVATE fakes ${STATE_INCLUDE}
        ${HC32}/Products/${product} ${HC32}/Platform/Config ${HC32}/Platform/Peripherals ${HC32}/Platform/System ${HC32}/Platform/HAL
        ${HC32}/Libraries/Bq40z50 ${HC32}/Libraries/Mp2762a ${HC32}/Libraries/Parser/include
        ${HC32}/Comm ${HC32}/Display)
    target_compile_options(${name}_power_tests PRIVATE -include ${CMAKE_CURRENT_LIST_DIR}/fakes/HAL.h)
    target_compile_definitions(${name}_power_tests PRIVATE __CORE_DEBUG __HAL_H POWER_TESTS_ONLY)
endforeach()
add_executable(gpio_tests gpio_tests.cpp
    ${HC32}/Platform/Peripherals/gpio.c ${HC32}/Platform/Peripherals/pinmap.cpp)
target_include_directories(gpio_tests PRIVATE gpio_fakes ${HC32}/Platform/Peripherals ${HC32}/Platform/System)
foreach(target gpio_tests firmware_tests x1_power_tests xbase_power_tests x1_protocol_tests xbase_protocol_tests clock_tests heap_tests)
    target_compile_options(${target} PRIVATE -Wall -Wextra -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    add_test(NAME ${target} COMMAND ${target})
endforeach()

# 带屏产品共享 LVGL 应用 + AppPlatform（固件路径）：固件 lv_conf.h（96 KB 池），C++ new 走 LVGL 池
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS ${ROOT}/External/lvgl/src/*.c)

# 三端自有显示/输入端口，软件渲染不编译硬件与第三方后端。
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/draw/(dma2d|espressif|eve|nanovg|nema_gfx|nxp|opengles|renesas|sdl|sifli|vg_lite)/")
list(FILTER LVGL_SOURCES EXCLUDE REGEX "/drivers/|/libs/")
add_library(display_lvgl STATIC ${LVGL_SOURCES})
target_include_directories(display_lvgl PUBLIC ${ROOT}/External/lvgl/include ${ROOT}/External/lvgl ${ROOT}/External ${HC32}/Display/Lvgl display_fakes)
target_compile_definitions(display_lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE LV_DISABLE_API_MAPPING)
add_library(app_lvgl INTERFACE)
target_link_libraries(app_lvgl INTERFACE display_lvgl)
add_library(app_build_options INTERFACE)
target_compile_options(app_build_options INTERFACE -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(app_build_options INTERFACE -fsanitize=address,undefined)
add_subdirectory(${ROOT}/App shared-app)
add_executable(page_manager_tests ${ROOT}/Vendor/Linux/tests/app_page_manager_test.cpp)
target_link_libraries(page_manager_tests PRIVATE app_page_manager)
target_compile_options(page_manager_tests PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME page_manager_tests COMMAND page_manager_tests)
add_executable(app_core_test ${ROOT}/App/tests/core_tests.cpp)
target_link_libraries(app_core_test PRIVATE app_core)
add_test(NAME app_core COMMAND app_core_test)
file(GLOB APP_PLATFORM_SOURCES CONFIGURE_DEPENDS ${HC32}/Display/AppPlatform/*.cpp)
foreach(product X1Pro XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_display_app_tests display_app_tests.cpp ${STATE_SOURCES}
        ${APP_PLATFORM_SOURCES} ${HC32}/Display/Lvgl/lvgl_new.cpp ${HC32}/Platform/HAL/HAL_Power.cpp)
    target_compile_options(${name}_display_app_tests PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-include;${CMAKE_CURRENT_LIST_DIR}/display_fakes/HAL.h>)
    target_compile_definitions(${name}_display_app_tests PRIVATE __HAL_H)
    # 统计真实后端的开机请求，验证提前松键从未调用 HAL；电源时序仍执行固件实现。
    target_link_options(${name}_display_app_tests PRIVATE -Wl,--wrap=_ZN3HAL15Power_RequestOnEv)
    # display_fakes 在前：替换固件 HAL 与 GPIO/时基接口；调试接口取自 fakes
    target_include_directories(${name}_display_app_tests PRIVATE display_fakes fakes ${STATE_INCLUDE}
        ${HC32}/Display ${HC32}/Display/AppPlatform ${HC32}/Products/${product} ${HC32}/Platform/Config
        ${HC32}/Platform/Peripherals ${HC32}/Platform/System ${HC32}/Platform/HAL)
    target_link_libraries(${name}_display_app_tests PRIVATE app_ui)
    target_compile_options(${name}_display_app_tests PRIVATE -Wall -Wextra -UNDEBUG)
    add_test(NAME ${name}_display_app_tests COMMAND ${name}_display_app_tests)
    # 固件 lv_conf 的断言处理是死循环：分配失败表现为超时
    set_tests_properties(${name}_display_app_tests PROPERTIES TIMEOUT 120)
endforeach()

# 原生面板与真实显示端口：逐字节/控制线核对、DMA 回收与产品偏移回归。
foreach(product X1Pro XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_spi_panel_tests spi_panel_tests.cpp
        ${HC32}/Platform/Peripherals/SpiMaster.cpp
        ${HC32}/Display/Panel/PanelRm690a0.cpp ${HC32}/Display/Panel/board_display.cpp ${HC32}/Display/Lvgl/lv_port_disp.cpp)
    target_include_directories(${name}_spi_panel_tests PRIVATE spi_fakes ${HC32}/Platform/Peripherals
        ${HC32}/Products/${product} ${HC32}/Platform/Config ${HC32}/Display ${HC32}/Display/Panel ${HC32}/Display/Lvgl)
    target_link_libraries(${name}_spi_panel_tests PRIVATE display_lvgl)
    target_compile_options(${name}_spi_panel_tests PRIVATE -Wall -Wextra -UNDEBUG
        -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${name}_spi_panel_tests PRIVATE -fsanitize=address,undefined
        -Wl,--wrap=_ZN9SpiMaster13startDmaWriteEPKvm)
    add_test(NAME ${name}_spi_panel_tests COMMAND ${name}_spi_panel_tests)
endforeach()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
# 系统状态写入口只由对应写入者调用
add_test(NAME state_ownership
    COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_LIST_DIR}/check_state_ownership.py ${ROOT})

set(SIMULATOR_EXECUTABLE "" CACHE FILEPATH "Already built LVGL simulator")
if(SIMULATOR_EXECUTABLE)
    add_test(NAME simulator_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/simulator_smoke.py ${SIMULATOR_EXECUTABLE})
endif()
