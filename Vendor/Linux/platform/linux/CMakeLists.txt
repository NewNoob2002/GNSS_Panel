add_executable(rk3506_app main.cpp)
add_executable(linux::app ALIAS rk3506_app)
target_link_libraries(rk3506_app PRIVATE linux::demo_backend linux::display linux::button linux::display_io host::log)
target_compile_options(rk3506_app PRIVATE -Wall -Wextra -Werror)
