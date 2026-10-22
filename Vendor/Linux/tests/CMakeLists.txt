add_executable(display_protocol_test display_protocol_test.c)
target_link_libraries(display_protocol_test PRIVATE lv_display)
target_compile_options(display_protocol_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME display_protocol COMMAND display_protocol_test)

add_executable(linux_display_test linux_display_test.c)
target_link_libraries(linux_display_test PRIVATE rm690a0)
target_compile_options(linux_display_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_link_options(linux_display_test PRIVATE
    -Wl,--wrap=open,--wrap=open64,--wrap=close,--wrap=ioctl,--wrap=flock,--wrap=nanosleep
    -Wl,--wrap=__ioctl_time64,--wrap=__nanosleep64
)
add_test(NAME linux_display COMMAND linux_display_test)

add_executable(app_core_test "${SHARED_APP_DIR}/tests/core_tests.cpp")
target_link_libraries(app_core_test PRIVATE app_core)
target_compile_options(app_core_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_core COMMAND app_core_test)

add_executable(app_framework_test app_framework_test.cpp)
target_link_libraries(app_framework_test PRIVATE app_framework app_log)
target_compile_options(app_framework_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_framework COMMAND app_framework_test)

add_executable(app_page_manager_test app_page_manager_test.cpp)
target_link_libraries(app_page_manager_test PRIVATE fw::pagemanager)
target_compile_options(app_page_manager_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_page_manager COMMAND app_page_manager_test)

add_executable(app_navigation_preview app_navigation_preview.cpp)
target_link_libraries(app_navigation_preview PRIVATE demo_backend)
target_compile_options(app_navigation_preview PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_navigation_preview COMMAND app_navigation_preview)

add_executable(app_status_test app_status_test.cpp)
target_link_libraries(app_status_test PRIVATE demo_backend)
target_compile_options(app_status_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_status COMMAND app_status_test)

add_executable(app_system_dash_test app_system_dash_test.cpp)
target_link_libraries(app_system_dash_test PRIVATE demo_backend)
target_compile_options(app_system_dash_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_system_dash COMMAND app_system_dash_test "${CMAKE_CURRENT_BINARY_DIR}/system-dash-pages")

# Platform-decided power flow with a test system_backend (no simulation library).
add_executable(app_platform_test app_platform_test.cpp)
target_link_libraries(app_platform_test PRIVATE app_ui)
target_compile_options(app_platform_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_platform COMMAND app_platform_test)

add_executable(app_dialplate_preview app_dialplate_preview.cpp)
target_link_libraries(app_dialplate_preview PRIVATE app_dialplate)
target_compile_options(app_dialplate_preview PRIVATE -Wall -Wextra -Werror -UNDEBUG)
add_test(NAME app_dialplate_preview COMMAND app_dialplate_preview)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(linux_button_test linux_button_test.c "${APPLICATION_DIR}/platform/linux_button.c")
    target_compile_options(linux_button_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
    add_test(NAME linux_button COMMAND linux_button_test)
endif()
