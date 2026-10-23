add_library(fw_databroker OBJECT DataNode.cpp DataBroker.cpp DataTimer.cpp)
add_library(fw::databroker ALIAS fw_databroker)
target_include_directories(fw_databroker PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(fw_databroker PUBLIC proj::options PRIVATE app::log)
target_compile_options(fw_databroker PRIVATE -Wall -Wextra -Werror
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)
