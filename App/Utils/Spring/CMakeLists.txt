add_library(app_spring STATIC Spring.cpp)
add_library(app::spring ALIAS app_spring)
target_include_directories(app_spring PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_spring PUBLIC proj::options)
target_compile_options(app_spring PRIVATE -Wall -Wextra -Werror
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
if(NOT CMAKE_CROSSCOMPILING)
    target_link_libraries(app_spring PUBLIC m)
endif()

if(TARGET lvgl::lvgl)
    add_library(app_lv_spring STATIC lv_spring.cpp)
    add_library(app::lv_spring ALIAS app_lv_spring)
    target_link_libraries(app_lv_spring PUBLIC app::spring lvgl::lvgl)
    target_include_directories(app_lv_spring PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(app_lv_spring PRIVATE -Wall -Wextra -Werror
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
endif()
