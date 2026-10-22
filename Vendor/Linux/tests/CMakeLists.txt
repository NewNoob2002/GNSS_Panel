add_executable(display_protocol_test display_protocol_test.c)
target_link_libraries(display_protocol_test PRIVATE linux::display)
target_compile_options(display_protocol_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME display_protocol COMMAND display_protocol_test)

add_executable(linux_display_test linux_display_test.c)
target_link_libraries(linux_display_test PRIVATE linux::display_io)
target_compile_options(linux_display_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_options(linux_display_test PRIVATE
    -Wl,--wrap=open,--wrap=open64,--wrap=close,--wrap=ioctl,--wrap=flock,--wrap=nanosleep
    -Wl,--wrap=__ioctl_time64,--wrap=__nanosleep64
)
add_test(NAME linux_display COMMAND linux_display_test)

include(${SHARED_APP_DIR}/tests/targets.cmake)
# 编译未注册的 MVP 页面模板，避免接口随框架变更失效。
add_subdirectory(${SHARED_APP_DIR}/UI/_Template app_page_template)
app_add_core_test(app_core_test)

add_executable(app_framework_test app_framework_test.cpp)
target_link_libraries(app_framework_test PRIVATE app::framework app::log)
target_compile_options(app_framework_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_framework COMMAND app_framework_test)

include(portable.cmake)
linux_add_page_manager_test(app_page_manager_test)

add_executable(app_navigation_preview app_navigation_preview.cpp)
target_link_libraries(app_navigation_preview PRIVATE linux::demo_backend)
target_compile_options(app_navigation_preview PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_navigation_preview COMMAND app_navigation_preview)

add_executable(app_status_test app_status_test.cpp)
target_link_libraries(app_status_test PRIVATE linux::demo_backend)
target_compile_options(app_status_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_status COMMAND app_status_test)

add_executable(hal_devices_test hal_devices_test.cpp)
target_link_libraries(hal_devices_test PRIVATE linux::demo_backend app::hal_test_checks)
target_compile_options(hal_devices_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME hal_devices COMMAND hal_devices_test)

add_executable(app_system_dash_test app_system_dash_test.cpp)
target_link_libraries(app_system_dash_test PRIVATE linux::demo_backend)
target_compile_options(app_system_dash_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_system_dash COMMAND app_system_dash_test "${CMAKE_CURRENT_BINARY_DIR}/system-dash-pages")

# Platform-decided power flow with a test system_backend (no simulation library).
add_executable(app_platform_test app_platform_test.cpp)
target_link_libraries(app_platform_test PRIVATE app::ui)
target_compile_options(app_platform_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_platform COMMAND app_platform_test)

add_executable(app_dialplate_preview app_dialplate_preview.cpp)
target_link_libraries(app_dialplate_preview PRIVATE app::dialplate)
target_compile_options(app_dialplate_preview PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_dialplate_preview COMMAND app_dialplate_preview)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(linux_button_test linux_button_test.c)
    target_link_libraries(linux_button_test PRIVATE linux::button)
    target_compile_options(linux_button_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    add_test(NAME linux_button COMMAND linux_button_test)
endif()
