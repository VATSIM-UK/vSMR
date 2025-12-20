# Logger Module
add_library(vSMR_Logger STATIC
    Logger.cpp
)

target_include_directories(vSMR_Logger
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/Logger
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_link_libraries(vSMR_Logger
    PUBLIC
        euroscope_interface
)
