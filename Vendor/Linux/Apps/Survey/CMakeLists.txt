add_executable(survey_app main.cpp)
add_executable(linux::survey ALIAS survey_app)
# 旧平台工具链待确认，入口只使用 C++98 标准库。
set_target_properties(survey_app PROPERTIES
    CXX_STANDARD 98
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
target_compile_options(survey_app PRIVATE -Wall -Wextra -Werror)
add_test(NAME survey_smoke COMMAND survey_app)
set_tests_properties(survey_smoke PROPERTIES PASS_REGULAR_EXPRESSION "^Survey: started")
