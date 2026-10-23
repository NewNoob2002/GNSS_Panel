file(GLOB APP_PLATFORM_SOURCES CONFIGURE_DEPENDS *.cpp)
add_library(hc32_${HC32_PREFIX}app_platform OBJECT ${APP_PLATFORM_SOURCES})
add_library(hc32::${HC32_PREFIX}app_platform ALIAS hc32_${HC32_PREFIX}app_platform)
target_include_directories(hc32_${HC32_PREFIX}app_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}app_platform PUBLIC
    ${HC32_PORT} app::ui hc32::${HC32_PREFIX}state hc32::${HC32_PREFIX}gpio_headers)
