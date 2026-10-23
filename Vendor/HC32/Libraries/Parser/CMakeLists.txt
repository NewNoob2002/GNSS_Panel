file(GLOB_RECURSE PARSER_SOURCES CONFIGURE_DEPENDS
    src/*.c
    src/*.cpp
)
add_library(hc32_parser OBJECT ${PARSER_SOURCES})
add_library(hc32::parser ALIAS hc32_parser)
target_include_directories(hc32_parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hc32_parser PUBLIC proj::options)
