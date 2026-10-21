add_library(hc32_arduino_headers INTERFACE)
target_include_directories(hc32_arduino_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/avr
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/variants/hc32f460xx
    ${PROJECT_SOURCE_DIR}/Libraries/easylogger/inc
)
target_link_libraries(hc32_arduino_headers INTERFACE
    hc32_project_options
    hc32_platform_headers
)

add_library(hc32_arduino OBJECT
    Core/avr/dtostrf.c
    Core/variants/hc32f460xx/variant.cpp
    Print.cpp
    Stream.cpp
    WMath.cpp
    WString.cpp
    itoa.c
    wiring_digital.cpp
    yield.c
)

target_link_libraries(hc32_arduino PUBLIC hc32_arduino_headers)
