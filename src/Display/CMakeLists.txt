# Display Module
add_library(vSMR_Display STATIC
    RadarDisplay.cpp
)

target_include_directories(vSMR_Display
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/Display
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_link_libraries(vSMR_Display
    PUBLIC
        euroscope_interface
        vSMR_Logger
)
