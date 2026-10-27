cmake_minimum_required(VERSION 3.20)
project(hc32_regressions LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

set(ROOT ${CMAKE_CURRENT_LIST_DIR}/..)
set(HC32 ${ROOT}/Vendor/HC32)

include(${ROOT}/cmake/options.cmake)
target_compile_options(project_options INTERFACE -Wall -Wextra -UNDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(project_options INTERFACE -fsanitize=address,undefined)

include(${HC32}/cmake/portable.cmake)

# Fake 硬件接口定义
add_library(test_fakes INTERFACE)
target_include_directories(test_fakes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fakes)
target_compile_definitions(test_fakes INTERFACE
    __CORE_DEBUG
    __HAL_H
)
target_link_libraries(test_fakes INTERFACE proj::options)

add_library(test_display_fakes INTERFACE)
target_include_directories(test_display_fakes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/display_fakes
)
target_compile_definitions(test_display_fakes INTERFACE
    __CORE_DEBUG
    __HAL_H
)
target_link_libraries(test_display_fakes INTERFACE proj::options)

add_library(test_spi_fakes INTERFACE)
target_include_directories(test_spi_fakes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/spi_fakes
)
target_link_libraries(test_spi_fakes INTERFACE proj::options)

add_library(test_time_fakes INTERFACE)
target_include_directories(test_time_fakes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/time_fakes)
target_link_libraries(test_time_fakes INTERFACE proj::options)

add_library(test_gpio_fakes INTERFACE)
target_include_directories(test_gpio_fakes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio_fakes
)
target_link_libraries(test_gpio_fakes INTERFACE proj::options)

# 实例化各产品测试目标
set(HC32_BUILD_LOG_TESTS TRUE)
hc32_add_portable("x1pro_" "X1Pro" test_fakes DRIVERS DISPLAY)
hc32_add_portable("x1pro_disp_" "X1Pro" test_display_fakes DISPLAY)
hc32_add_portable("x1pro_spi_" "X1Pro" test_spi_fakes SPI)
hc32_add_portable("xbase_" "XBase" test_fakes DRIVERS DISPLAY)
hc32_add_portable("xbase_disp_" "XBase" test_display_fakes DISPLAY)
hc32_add_portable("xbase_spi_" "XBase" test_spi_fakes SPI)
hc32_add_portable("x1_" "X1" test_fakes DRIVERS)
hc32_add_portable("x1_gpio_" "X1" test_gpio_fakes GPIO)
hc32_add_portable("clock_" "X1Pro" test_time_fakes CLOCK)
hc32_add_portable("heap_" "X1Pro" proj::options HEAP)

# 1. firmware_tests
add_executable(firmware_tests firmware_tests.cpp)
target_link_libraries(firmware_tests PRIVATE
    test_fakes
    hc32::x1pro_hal
    hc32::x1pro_power_policy
    hc32::x1pro_state
    hc32::x1pro_board
    hc32::x1pro_peripherals
    hc32::x1pro_devices
    hc32::x1pro_comm
    hc32::x1pro_display_protocol
    hc32::parser
)
add_test(NAME firmware_tests COMMAND firmware_tests)

# 2. Protocol tests per product
add_executable(x1_protocol_tests product_protocol_tests.cpp)
target_link_libraries(x1_protocol_tests PRIVATE
    hc32::x1_state
    hc32::x1_protocol
    hc32::x1_product_x1
    hc32::parser
    test_fakes
)
add_test(NAME x1_protocol_tests COMMAND x1_protocol_tests)

add_executable(xbase_protocol_tests product_protocol_tests.cpp)
target_link_libraries(xbase_protocol_tests PRIVATE
    hc32::xbase_state
    hc32::xbase_protocol
    hc32::xbase_display_protocol
    hc32::parser
    test_fakes
)
add_test(NAME xbase_protocol_tests COMMAND xbase_protocol_tests)

# 3. Clock & heap tests
add_executable(clock_tests clock_tests.c)
target_link_libraries(clock_tests PRIVATE hc32::clock_system test_time_fakes hc32::clock_system_port)
add_test(NAME clock_tests COMMAND clock_tests)

add_executable(heap_tests heap_tests.c)
target_compile_definitions(heap_tests PRIVATE _DEFAULT_SOURCE)
target_link_libraries(heap_tests PRIVATE hc32::heap_system hc32::segger_rtt hc32::heap_system_port)
add_test(NAME heap_tests COMMAND heap_tests)

# 4. Power tests
foreach(product X1 XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_power_tests firmware_tests.cpp)
    target_compile_definitions(${name}_power_tests PRIVATE POWER_TESTS_ONLY)
    target_link_libraries(${name}_power_tests PRIVATE
        test_fakes
        hc32::${name}_hal
        hc32::${name}_power_policy
        hc32::${name}_state
        hc32::${name}_board
        hc32::${name}_peripherals
        hc32::${name}_devices
    )
    add_test(NAME ${name}_power_tests COMMAND ${name}_power_tests)
endforeach()

# 5. GPIO tests
add_executable(gpio_tests gpio_tests.cpp)
target_link_libraries(gpio_tests PRIVATE hc32::x1_gpio_peripherals test_gpio_fakes hc32::x1_gpio_peripherals_port)
add_test(NAME gpio_tests COMMAND gpio_tests)

# 6. LVGL and Shared App setup
include(${ROOT}/cmake/lvgl.cmake)
project_add_lvgl("${HC32}/Display/Lvgl/lv_conf.h")
target_link_libraries(lvgl PUBLIC proj::options)

add_subdirectory(${ROOT}/App shared-app)

include(${ROOT}/Vendor/Linux/tests/portable.cmake)
linux_add_page_manager_test(page_manager_tests)

include(${ROOT}/App/tests/targets.cmake)
app_add_core_test(app_core_test)

# HC32 日志桥接与发布版禁用回归
add_subdirectory(${HC32}/Libraries/easylogger hc32_easylogger)
add_executable(hc32_log_sink_tests hc32_log_sink_tests.cpp)
target_link_libraries(hc32_log_sink_tests PRIVATE app::log hc32::app_log_headers hc32::easylogger_headers)
add_test(NAME hc32_log_sink_tests COMMAND hc32_log_sink_tests)

add_executable(hc32_log_disabled_tests hc32_log_disabled_tests.cpp)
target_link_libraries(hc32_log_disabled_tests PRIVATE hc32::app_log_disabled proj::options)
add_test(NAME hc32_log_disabled_tests COMMAND hc32_log_disabled_tests)

# 7. Display app integration tests
foreach(product X1Pro XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_display_app_tests display_app_tests.cpp domain_device_checks.cpp)
    target_link_libraries(${name}_display_app_tests PRIVATE
        hc32::${name}_disp_app_platform
        hc32::${name}_disp_lvgl_port
        hc32::${name}_disp_power_policy
        hc32::${name}_disp_state
        app::ui
        app::hal_test_checks
        test_display_fakes
    )
    target_link_options(${name}_display_app_tests PRIVATE -Wl,--wrap=_ZN3HAL15Power_RequestOnEv)
    add_test(NAME ${name}_display_app_tests COMMAND ${name}_display_app_tests)
    set_tests_properties(${name}_display_app_tests PROPERTIES TIMEOUT 120)
endforeach()

# 8. SPI panel tests
foreach(product X1Pro XBase)
    string(TOLOWER ${product} name)
    add_executable(${name}_spi_panel_tests spi_panel_tests.cpp)
    target_link_libraries(${name}_spi_panel_tests PRIVATE
        hc32::${name}_spi_panel
        hc32::${name}_spi_lvgl_port
        hc32::${name}_spi_peripherals
        lvgl::lvgl
        test_spi_fakes
    )
    target_link_options(${name}_spi_panel_tests PRIVATE -Wl,--wrap=_ZN9SpiMaster13startDmaWriteEPKvm)
    add_test(NAME ${name}_spi_panel_tests COMMAND ${name}_spi_panel_tests)
endforeach()

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(NAME state_ownership
    COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_LIST_DIR}/check_state_ownership.py ${ROOT})
add_test(NAME check_layers
    COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_LIST_DIR}/check_layers.py ${ROOT})

set(SIMULATOR_EXECUTABLE "" CACHE FILEPATH "Already built LVGL simulator")
if(SIMULATOR_EXECUTABLE)
    add_test(NAME simulator_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/simulator_smoke.py ${SIMULATOR_EXECUTABLE})
endif()
