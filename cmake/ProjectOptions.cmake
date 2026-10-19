# 将工具链的隐式系统头路径写入编译数据库，供 clangd 使用。
set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_C_IMPLICIT_INCLUDE_DIRECTORIES})
set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_library(hc32_project_options INTERFACE)
target_include_directories(hc32_project_options INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config
    # 所选产品的 product_config.h
    ${PRODUCT_DIR}
)
target_compile_definitions(hc32_project_options INTERFACE
    HC32F460
    USE_DDL_DRIVER
    ARDUINO=111
    LV_CONF_INCLUDE_SIMPLE
    $<$<CONFIG:Debug>:__CORE_DEBUG>
    # MIDDLE_LOGO_USE
)
target_compile_options(hc32_project_options INTERFACE
    ${CPU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<CONFIG:Debug>:-Og -g3>
    $<$<CONFIG:Release>:-Os -g0>
)
