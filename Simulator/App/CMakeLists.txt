file(GLOB_RECURSE SIMULATOR_APP_SOURCES CONFIGURE_DEPENDS
    *.c
    *.cpp
)

add_library(hc32_simulator_app_headers INTERFACE)
target_include_directories(hc32_simulator_app_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Common
    ${CMAKE_CURRENT_SOURCE_DIR}/Common/DataProc
    ${CMAKE_CURRENT_SOURCE_DIR}/Pages
    ${CMAKE_CURRENT_SOURCE_DIR}/Resource
    ${CMAKE_CURRENT_SOURCE_DIR}/Utils
)
target_link_libraries(hc32_simulator_app_headers INTERFACE
    hc32_project_options
    hc32_platform_headers
    hc32_libraries_headers
    hc32_display_libraries
)

add_library(hc32_simulator_app OBJECT
    ${SIMULATOR_APP_SOURCES}
)

target_link_libraries(hc32_simulator_app PUBLIC hc32_simulator_app_headers)
