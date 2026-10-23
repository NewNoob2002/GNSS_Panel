add_library(fw_resourcemanager INTERFACE)
add_library(fw::resourcemanager ALIAS fw_resourcemanager)
target_include_directories(fw_resourcemanager INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
