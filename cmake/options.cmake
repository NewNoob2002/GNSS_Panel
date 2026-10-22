include_guard(GLOBAL)
if(NOT TARGET project_options)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}" "${CMAKE_BINARY_DIR}/_cmake_options" EXCLUDE_FROM_ALL)
endif()
