# OEM 生成物属于应用层；无屏产品也复用此选择校验，不引入 UI 依赖。
set(OEM "singularxyz" CACHE STRING "OEM resource directory")
set(_oem_root "${CMAKE_CURRENT_LIST_DIR}/UI/Resource/oem")
file(GLOB _oem_configs RELATIVE "${_oem_root}" "${_oem_root}/*/oem.toml")
set(_oem_available)
foreach(_config IN LISTS _oem_configs)
    get_filename_component(_name "${_config}" DIRECTORY)
    list(APPEND _oem_available "${_name}")
endforeach()
set_property(CACHE OEM PROPERTY STRINGS ${_oem_available})
set(APP_OEM_DIR "${_oem_root}/${OEM}")
if(NOT OEM IN_LIST _oem_available OR
   NOT EXISTS "${APP_OEM_DIR}/generated/oem_logo.c" OR
   NOT EXISTS "${APP_OEM_DIR}/generated/oem_config.h")
    message(FATAL_ERROR "Unknown OEM '${OEM}' or missing generated resources. Available OEM: ${_oem_available}. Run App/UI/Resource/tools/oem_build.py for the selected directory.")
endif()
message(STATUS "OEM: ${OEM}")
