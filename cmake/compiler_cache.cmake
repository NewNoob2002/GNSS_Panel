# 各 preset 使用独立构建树，X1Pro/XBase 的 LVGL 与 App 编译命令却完全相同。
# 由 ccache 按预处理结果复用对象：比手工抽取公共库更安全，产品开关一旦改变内容，
# 对应对象会自动重新编译，不会链接到陈旧的库。
include_guard(GLOBAL)

option(PROJECT_USE_CCACHE "Reuse compiler output across build directories via ccache" ON)
if(NOT PROJECT_USE_CCACHE)
    return()
endif()
find_program(CCACHE_PROGRAM ccache)
if(NOT CCACHE_PROGRAM)
    message(STATUS "ccache not found: every build directory compiles from scratch")
    return()
endif()
message(STATUS "Compiler cache: ${CCACHE_PROGRAM}")
# CMake 不为 ASM 规则应用 launcher，启动文件等汇编源每个构建目录都会重新汇编。
foreach(lang C CXX)
    set(CMAKE_${lang}_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endforeach()
# ccache 对 -g 构建会把工作目录计入哈希，GCC 也会把它写进预处理输出；
# 去掉这两处后 Debug 才能跨 preset 命中，且调试信息不再含构建目录。
# 需探测支持情况：旧交叉工具链（如 GCC 4.9.3）没有 -ffile-prefix-map，此时缓存仅在同目录内生效。
include(CheckCCompilerFlag)
check_c_compiler_flag(-ffile-prefix-map=/a=. PROJECT_HAS_FILE_PREFIX_MAP)
check_c_compiler_flag(-fno-working-directory PROJECT_HAS_NO_WORKING_DIRECTORY)
if(PROJECT_HAS_FILE_PREFIX_MAP AND PROJECT_HAS_NO_WORKING_DIRECTORY)
    add_compile_options("-ffile-prefix-map=${CMAKE_BINARY_DIR}=." -fno-working-directory)
endif()
