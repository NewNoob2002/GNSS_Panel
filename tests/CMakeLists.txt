cmake_minimum_required(VERSION 3.20)
project(hc32_regressions LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
enable_testing()
set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
# 系统状态分域（所有使用 systemInfo 的测试都链接它）
set(STATE_SOURCES ${ROOT}/Platform/State/SystemState.cpp)
set(STATE_INCLUDE ${ROOT}/Platform/State)
set(PARSER_SOURCES
    ${ROOT}/Libraries/Parser/src/SparkFun_Extensible_Message_Parser.cpp
    ${ROOT}/Libraries/Parser/src/Parse_CUSTOM.cpp)
# 传输层、电源与 X1Pro 协议表（带屏产品基线）
add_executable(firmware_tests firmware_tests.cpp ${STATE_SOURCES}
    ${ROOT}/Shared/Comm/slave_i2c.cpp ${ROOT}/Shared/Comm/panel_protocol.cpp
    ${ROOT}/Shared/Display/display_protocol.cpp
    ${PARSER_SOURCES}
    ${ROOT}/Platform/Core/src/Wire.cpp ${ROOT}/Platform/HAL/HAL_Power.cpp ${ROOT}/Platform/HAL/Charger.cpp
    ${ROOT}/Libraries/Bq40z50/bq40z50.cpp ${ROOT}/Libraries/Mp2762a/mp2762a.cpp)
target_include_directories(firmware_tests PRIVATE fakes ${STATE_INCLUDE} ${ROOT}/Shared/Comm ${ROOT}/Shared/Display
    ${ROOT}/Products/X1Pro ${ROOT}/Platform/Config
    ${ROOT}/Platform/Core/inc ${ROOT}/Platform/HAL ${ROOT}/Libraries/Parser/include
    ${ROOT}/Libraries/Bq40z50 ${ROOT}/Libraries/Mp2762a)

# 各产品协议表：同一测试源分别按产品配置编译
add_executable(x1_protocol_tests product_protocol_tests.cpp ${STATE_SOURCES}
    ${ROOT}/Shared/Comm/panel_protocol.cpp ${ROOT}/Products/X1/x1_protocol.cpp ${PARSER_SOURCES})
target_include_directories(x1_protocol_tests PRIVATE fakes ${STATE_INCLUDE} ${ROOT}/Shared/Comm ${ROOT}/Products/X1
    ${ROOT}/Platform/Config ${ROOT}/Libraries/Parser/include)
add_executable(xbase_protocol_tests product_protocol_tests.cpp ${STATE_SOURCES}
    ${ROOT}/Shared/Comm/panel_protocol.cpp ${ROOT}/Shared/Display/display_protocol.cpp ${PARSER_SOURCES})
target_include_directories(xbase_protocol_tests PRIVATE fakes ${STATE_INCLUDE} ${ROOT}/Shared/Comm ${ROOT}/Shared/Display
    ${ROOT}/Products/XBase ${ROOT}/Platform/Config ${ROOT}/Libraries/Parser/include)
target_compile_options(firmware_tests PRIVATE -include ${CMAKE_CURRENT_LIST_DIR}/fakes/HAL.h)
target_compile_definitions(firmware_tests PRIVATE __CORE_DEBUG __HAL_H)
add_executable(clock_tests clock_tests.c)
target_include_directories(clock_tests PRIVATE time_fakes)
add_executable(heap_tests heap_tests.c ${ROOT}/Platform/Core/src/Syscalls_GCC.c)
target_compile_definitions(heap_tests PRIVATE _DEFAULT_SOURCE)
foreach(target firmware_tests x1_protocol_tests xbase_protocol_tests clock_tests heap_tests)
    target_compile_options(${target} PRIVATE -Wall -Wextra -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    add_test(NAME ${target} COMMAND ${target})
endforeach()

# 带屏产品共享 LVGL 应用 + AppPlatform（固件路径）：固件 lv_conf.h（96 KB 池），C++ new 走 LVGL 池
file(GLOB_RECURSE SHARED_APP_SOURCES CONFIGURE_DEPENDS ${ROOT}/Simulator/App/*.c ${ROOT}/Simulator/App/*.cpp)
list(FILTER SHARED_APP_SOURCES EXCLUDE REGEX "/Status/")
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS ${ROOT}/Libraries/lvgl/src/*.c)
add_library(display_lvgl STATIC ${LVGL_SOURCES})
target_include_directories(display_lvgl PUBLIC ${ROOT}/Libraries/lvgl ${ROOT}/Libraries display_fakes)
target_compile_definitions(display_lvgl PUBLIC LV_CONF_INCLUDE_SIMPLE)
file(GLOB APP_PLATFORM_SOURCES CONFIGURE_DEPENDS ${ROOT}/Shared/Display/AppPlatform/*.cpp)
foreach(product X1Pro XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_display_app_tests display_app_tests.cpp ${SHARED_APP_SOURCES} ${STATE_SOURCES}
        ${APP_PLATFORM_SOURCES} ${ROOT}/Shared/Display/lvgl_new.cpp)
    # display_fakes 在前：替换固件的 HAL.h / Arduino.h；core_debug.h 取自 fakes
    target_include_directories(${name}_display_app_tests PRIVATE display_fakes fakes ${STATE_INCLUDE} ${ROOT}/Simulator/App
        ${ROOT}/Shared/Display ${ROOT}/Products/${product} ${ROOT}/Platform/Config)
    target_link_libraries(${name}_display_app_tests PRIVATE display_lvgl)
    target_compile_options(${name}_display_app_tests PRIVATE -Wall -Wextra -UNDEBUG)
    add_test(NAME ${name}_display_app_tests COMMAND ${name}_display_app_tests)
    # 固件 lv_conf 的断言处理是死循环：分配失败表现为超时
    set_tests_properties(${name}_display_app_tests PROPERTIES TIMEOUT 120)
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
