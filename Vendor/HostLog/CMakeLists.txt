# 仅 Linux/Simulator 使用；上游源码保持完整、未修改。
# spdlog v1.17.0，MIT（External/spdlog/LICENSE），含 bundled fmt 许可证。
# https://github.com/gabime/spdlog/releases/tag/v1.17.0
# https://codeload.github.com/gabime/spdlog/tar.gz/refs/tags/v1.17.0
# tar.gz SHA256: d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744
# 应用接口传递 -fno-exceptions，使用上游同名选项保持 ABI/头文件配置一致。
set(SPDLOG_NO_EXCEPTIONS ON CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_ALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../../External/spdlog" "${CMAKE_BINARY_DIR}/spdlog")

add_library(host_log STATIC HostLog.cpp)
add_library(host::log ALIAS host_log)
target_include_directories(host_log PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(host_log PUBLIC app::log spdlog::spdlog)
target_compile_options(host_log PRIVATE -Wall -Wextra -Werror)
