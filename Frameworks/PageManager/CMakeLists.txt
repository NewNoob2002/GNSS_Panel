add_library(app_page_manager STATIC
    PageBase.cpp
    PageManager.cpp
    PageManagerRouter.cpp
    PageManagerState.cpp
    PageManagerAnim.cpp)
add_library(fw::pagemanager ALIAS app_page_manager)
target_include_directories(app_page_manager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(app_page_manager PUBLIC lvgl::lvgl proj::options PRIVATE app::log)
target_compile_options(app_page_manager PRIVATE -Wall -Wextra)
# PageBase 与 app_ui 中的派生页面必须使用一致的 RTTI 布局及检查策略。
target_compile_options(app_page_manager PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
