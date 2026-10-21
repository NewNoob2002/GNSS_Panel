cmake_minimum_required(VERSION 3.20)
# Parser 组件
# 消息解析器组件，支持多种协议解析
# 收集源文件
file(GLOB_RECURSE PARSER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

# 创建组件库
add_library(utils_parser STATIC ${PARSER_SOURCES})

# 设置包含目录
target_include_directories(utils_parser
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>
)

# 设置 C++ 标准
set_target_properties(utils_parser PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED ON
)

# 创建别名
add_library(utils::parser ALIAS utils_parser)

message(STATUS "  - Parser 组件已配置")