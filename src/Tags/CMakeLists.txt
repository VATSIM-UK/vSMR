add_library(vSMR_Tags STATIC
    Tag.cpp
)

target_include_directories(vSMR_Tags
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/Tags
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_link_libraries(vSMR_Tags
    PUBLIC
        euroscope_interface
)
