cmake_minimum_required(VERSION 3.20)

project(vSMR LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(vSMR SHARED 
    src/SMRPlugin.cpp
)

target_include_directories(vSMR PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc 
    ${CMAKE_CURRENT_SOURCE_DIR}/euroscope
)

if(WIN32)
    target_link_libraries(vSMR PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/euroscope/EuroScopePlugInDll.lib
    )
endif()

set_target_properties(vSMR PROPERTIES 
    OUTPUT_NAME "vSMR"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS vSMR DESTINATION bin)