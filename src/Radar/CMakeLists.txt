add_library(vSMR_Radar STATIC
    RadarDisplay.cpp
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
)