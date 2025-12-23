add_library(vSMR_Display STATIC
    MenuBar.cpp
)

target_include_directories(vSMR_Display PUBLIC
    ${CMAKE_SOURCE_DIR}/inc
    ${CMAKE_SOURCE_DIR}/euroscope/inc
)

target_link_libraries(vSMR_Display PUBLIC
)
