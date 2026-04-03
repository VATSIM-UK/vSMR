add_library(vSMR_Tags STATIC
    Tag.cpp
    TagProfileManager.cpp
)

target_include_directories(vSMR_Tags
    PUBLIC
        ${CMAKE_SOURCE_DIR}/inc/Tags
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
        ${CMAKE_SOURCE_DIR}/inc/Logger
)

target_link_libraries(vSMR_Tags
    PUBLIC
        euroscope_interface
        nlohmann_json::nlohmann_json
        vSMR_Logger
)
