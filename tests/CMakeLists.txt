cmake_minimum_required(VERSION 3.20)
project(hc32_regressions LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
enable_testing()
set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(PARSER_SOURCES
    ${ROOT}/Libraries/Parser/src/SparkFun_Extensible_Message_Parser.cpp
    ${ROOT}/Libraries/Parser/src/Parse_CUSTOM.cpp)
# 传输层、电源与 X1Pro 协议表（带屏产品基线）
add_executable(firmware_tests firmware_tests.cpp
    ${ROOT}/Shared/Comm/slave_i2c.cpp ${ROOT}/Shared/Comm/panel_protocol.cpp
    ${ROOT}/Shared/Display/display_protocol.cpp
    ${PARSER_SOURCES}
    ${ROOT}/Platform/Core/src/Wire.cpp ${ROOT}/Platform/HAL/HAL_Power.cpp
    ${ROOT}/Libraries/Bq40z50/bq40z50.cpp ${ROOT}/Libraries/Mp2762a/mp2762a.cpp)
target_include_directories(firmware_tests PRIVATE fakes ${ROOT}/Shared/Comm ${ROOT}/Shared/Display
    ${ROOT}/Products/X1Pro ${ROOT}/Platform/Config
    ${ROOT}/Platform/Core/inc ${ROOT}/Platform/HAL ${ROOT}/Libraries/Parser/include
    ${ROOT}/Libraries/Bq40z50 ${ROOT}/Libraries/Mp2762a)

# 各产品协议表：同一测试源分别按产品配置编译
add_executable(x1_protocol_tests product_protocol_tests.cpp
    ${ROOT}/Shared/Comm/panel_protocol.cpp ${ROOT}/Products/X1/x1_protocol.cpp ${PARSER_SOURCES})
target_include_directories(x1_protocol_tests PRIVATE fakes ${ROOT}/Shared/Comm ${ROOT}/Products/X1
    ${ROOT}/Platform/Config ${ROOT}/Libraries/Parser/include)
add_executable(xbase_protocol_tests product_protocol_tests.cpp
    ${ROOT}/Shared/Comm/panel_protocol.cpp ${ROOT}/Shared/Display/display_protocol.cpp ${PARSER_SOURCES})
target_include_directories(xbase_protocol_tests PRIVATE fakes ${ROOT}/Shared/Comm ${ROOT}/Shared/Display
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

set(SIMULATOR_EXECUTABLE "" CACHE FILEPATH "Already built LVGL simulator")
if(SIMULATOR_EXECUTABLE)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME simulator_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/simulator_smoke.py ${SIMULATOR_EXECUTABLE})
endif()
