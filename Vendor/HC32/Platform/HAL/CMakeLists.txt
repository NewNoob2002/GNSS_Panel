add_library(hc32_${HC32_PREFIX}hal_port INTERFACE)
add_library(hc32::${HC32_PREFIX}hal_port ALIAS hc32_${HC32_PREFIX}hal_port)
target_include_directories(hc32_${HC32_PREFIX}hal_port INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/port
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(hc32_${HC32_PREFIX}hal_port INTERFACE
    proj::options hc32::${HC32_PREFIX}config_headers)

add_library(hc32_${HC32_PREFIX}power_policy OBJECT HAL_Power.cpp)
add_library(hc32::${HC32_PREFIX}power_policy ALIAS hc32_${HC32_PREFIX}power_policy)
target_include_directories(hc32_${HC32_PREFIX}power_policy PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hc32_${HC32_PREFIX}power_policy PUBLIC
    ${HC32_PORT} hc32::${HC32_PREFIX}state hc32::${HC32_PREFIX}gpio_headers)

if(NOT HC32_PORTABLE)
    file(GLOB HAL_SOURCES CONFIGURE_DEPENDS *.c *.cpp)
    list(FILTER HAL_SOURCES EXCLUDE REGEX "HAL_Power[.]cpp$")
elseif(PORTABLE_DRIVERS)
    set(HAL_SOURCES HAL_Battery.cpp Charger.cpp)
endif()
if(HAL_SOURCES)
    add_library(hc32_${HC32_PREFIX}hal OBJECT ${HAL_SOURCES})
    add_library(hc32::${HC32_PREFIX}hal ALIAS hc32_${HC32_PREFIX}hal)
    target_include_directories(hc32_${HC32_PREFIX}hal PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(hc32_${HC32_PREFIX}hal PUBLIC ${HC32_PORT} hc32::${HC32_PREFIX}power_policy
        hc32::${HC32_PREFIX}board hc32::${HC32_PREFIX}devices)
    if(NOT HC32_PORTABLE)
        target_link_libraries(hc32_${HC32_PREFIX}hal PUBLIC hc32::system hc32::millis_task)
    endif()
endif()
