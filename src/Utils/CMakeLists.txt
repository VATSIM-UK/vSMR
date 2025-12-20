# Display Module
add_library(vSMR_Utils STATIC
    stringUtils.cpp
)

target_include_directories(vSMR_Utils
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/Utils
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_link_libraries(vSMR_Utils
    PUBLIC
)