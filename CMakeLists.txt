cmake_minimum_required(VERSION 3.20)

project(HC32F460xE_Arduino LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(HC32_HOST_TESTS_ONLY "Build host tests without the ARM firmware" OFF)
if(HC32_HOST_TESTS_ONLY)
    include(CTest)
    add_subdirectory(tests/host)
    return()
endif()

set(CLANGD_DATABASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/clangd")
if(IS_SYMLINK "${CLANGD_DATABASE_DIR}")
    file(REMOVE "${CLANGD_DATABASE_DIR}")
endif()
file(MAKE_DIRECTORY "${CLANGD_DATABASE_DIR}")
add_custom_target(clangd_database ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json"
    "${CLANGD_DATABASE_DIR}/compile_commands.json"
    COMMENT "clangd compilation database -> ${CMAKE_CURRENT_BINARY_DIR}"
)

set(PROJECT_VERSION 2.4.6)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Platform/Config/mcu_version_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config/mcu_version_config.h
    @ONLY
)
set(TARGET_NAME MCU_PRO_${PROJECT_VERSION})
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/HC32F460xE.ld)

get_filename_component(C_COMPILER_NAME "${CMAKE_C_COMPILER}" NAME)
if(C_COMPILER_NAME MATCHES "^arm-none-eabi-gcc")
    set(ARM_GCC_SYSTEM_INCLUDE_DIRS)
    set(ARM_GXX_SYSTEM_INCLUDE_DIRS)

    foreach(GCC_INCLUDE_QUERY IN ITEMS -print-file-name=include -print-file-name=include-fixed)
        execute_process(
            COMMAND ${CMAKE_C_COMPILER} ${GCC_INCLUDE_QUERY}
            OUTPUT_VARIABLE GCC_INCLUDE_DIR
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(GCC_INCLUDE_DIR AND EXISTS "${GCC_INCLUDE_DIR}")
            list(APPEND ARM_GCC_SYSTEM_INCLUDE_DIRS "${GCC_INCLUDE_DIR}")
        endif()
    endforeach()

    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-sysroot
        OUTPUT_VARIABLE ARM_GCC_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(ARM_GCC_SYSROOT AND EXISTS "${ARM_GCC_SYSROOT}/include")
        list(APPEND ARM_GCC_SYSTEM_INCLUDE_DIRS "${ARM_GCC_SYSROOT}/include")
    endif()

    execute_process(
        COMMAND ${CMAKE_C_COMPILER} -print-file-name=libc.a
        OUTPUT_VARIABLE ARM_GCC_LIBC
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(ARM_GCC_LIBC AND EXISTS "${ARM_GCC_LIBC}")
        get_filename_component(ARM_GCC_LIBC "${ARM_GCC_LIBC}" REALPATH)
        get_filename_component(ARM_GCC_LIB_DIR "${ARM_GCC_LIBC}" DIRECTORY)
        get_filename_component(ARM_GCC_TOOLCHAIN_ROOT "${ARM_GCC_LIB_DIR}" DIRECTORY)
        if(EXISTS "${ARM_GCC_TOOLCHAIN_ROOT}/include")
            list(APPEND ARM_GCC_SYSTEM_INCLUDE_DIRS "${ARM_GCC_TOOLCHAIN_ROOT}/include")
        endif()
    endif()

    if(CMAKE_CXX_COMPILER)
        execute_process(
            COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
            OUTPUT_VARIABLE ARM_GXX_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        set(ARM_GXX_INCLUDE_DIR "${ARM_GCC_TOOLCHAIN_ROOT}/include/c++/${ARM_GXX_VERSION}")
        foreach(ARM_GXX_SYSTEM_INCLUDE_DIR IN ITEMS
            "${ARM_GXX_INCLUDE_DIR}"
            "${ARM_GXX_INCLUDE_DIR}/arm-none-eabi"
            "${ARM_GXX_INCLUDE_DIR}/backward")
            if(EXISTS "${ARM_GXX_SYSTEM_INCLUDE_DIR}")
                list(APPEND ARM_GXX_SYSTEM_INCLUDE_DIRS "${ARM_GXX_SYSTEM_INCLUDE_DIR}")
            endif()
        endforeach()
    endif()

    set(ARM_GCC_SYSTEM_INCLUDE_OPTIONS)
    if(ARM_GCC_SYSTEM_INCLUDE_DIRS)
        list(REMOVE_DUPLICATES ARM_GCC_SYSTEM_INCLUDE_DIRS)
        foreach(ARM_SYSTEM_INCLUDE_DIR IN LISTS ARM_GCC_SYSTEM_INCLUDE_DIRS)
            list(APPEND ARM_GCC_SYSTEM_INCLUDE_OPTIONS "SHELL:-isystem \"${ARM_SYSTEM_INCLUDE_DIR}\"")
        endforeach()
        add_compile_options("$<$<COMPILE_LANGUAGE:C,ASM>:${ARM_GCC_SYSTEM_INCLUDE_OPTIONS}>")
    endif()

    if(ARM_GXX_SYSTEM_INCLUDE_DIRS OR ARM_GCC_SYSTEM_INCLUDE_DIRS)
        set(ARM_GXX_SYSTEM_INCLUDE_OPTIONS)
        foreach(ARM_SYSTEM_INCLUDE_DIR IN LISTS ARM_GXX_SYSTEM_INCLUDE_DIRS ARM_GCC_SYSTEM_INCLUDE_DIRS)
            list(APPEND ARM_GXX_SYSTEM_INCLUDE_OPTIONS "SHELL:-isystem \"${ARM_SYSTEM_INCLUDE_DIR}\"")
        endforeach()
        add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:${ARM_GXX_SYSTEM_INCLUDE_OPTIONS}>")
        message(STATUS "ARM GCC system includes: ${ARM_GCC_SYSTEM_INCLUDE_DIRS};${ARM_GXX_SYSTEM_INCLUDE_DIRS}")
    endif()
endif()

set(CPU_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

add_library(hc32_project_options INTERFACE)
target_include_directories(hc32_project_options INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/generated/Platform/Config
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

add_subdirectory(Platform)
add_subdirectory(Arduino)
add_subdirectory(Libraries)
add_subdirectory(Simulator/App)
add_subdirectory(USER)

add_executable(${TARGET_NAME})
target_link_libraries(${TARGET_NAME} PRIVATE
    hc32_platform
    hc32_arduino
    hc32_libraries
    hc32_simulator_app
    hc32_user
)
target_link_options(${TARGET_NAME} PRIVATE
    ${CPU_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--defsym=FLASH_ORIGIN=0x00008000
    # -Wl,--defsym=FLASH_LENGTH=0x00036000
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    --specs=nano.specs
    --specs=nosys.specs
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generating HEX/BIN and size report"
)
