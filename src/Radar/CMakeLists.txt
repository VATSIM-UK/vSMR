add_library(vSMR_Radar STATIC
    RadarDisplay.cpp
    AircraftRenderer.cpp
)

target_include_directories(vSMR_Radar
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/Radar
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_link_libraries(vSMR_Radar
    PUBLIC
        euroscope_interface
        vSMR_Logger
        vSMR_Display
        vSMR_Tags
        vSMR_Utils
)