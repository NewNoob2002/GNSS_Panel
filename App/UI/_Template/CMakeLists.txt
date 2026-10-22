# 模板只在 Linux 主机测试构建中编译，不安装到页面清单，也不进入固件。
add_library(app_page_template OBJECT Template.cpp TemplateModel.cpp TemplateView.cpp)
target_link_libraries(app_page_template PRIVATE app::ui)
target_compile_options(app_page_template PRIVATE -Wall -Wextra -Werror)
