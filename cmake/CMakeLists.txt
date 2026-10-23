add_library(project_options INTERFACE)
add_library(proj::options ALIAS project_options)
target_compile_features(project_options INTERFACE c_std_11 cxx_std_17)
target_compile_options(project_options INTERFACE -Wall -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
