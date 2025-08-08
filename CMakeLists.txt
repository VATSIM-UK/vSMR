cmake_minimum_required(VERSION 3.15)

project(vSMR LANGUAGES CXX)

set(CMAKE_GENERATOR_PLATFORM "Win32")
set(CMAKE_VS_PLATFORM_NAME "Win32")


list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}")

find_package(asio CONFIG REQUIRED)
find_package(CURL CONFIG REQUIRED)
#find_package(ZLIB CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)

set(CMAKE_MFC_FLAG 1) # Use static MFC

# Source files
set(SOURCES
    ${CMAKE_SOURCE_DIR}/vSMR/SMRPlugin.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/SMRRadar.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/Config.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/CallsignLookup.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/AircraftTypeLookup.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/InsetWindow.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/DataLinkDialog.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/CPDLCSettingsDialog.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/HttpHelper.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/Rimcas.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/vSMR.cpp
    ${CMAKE_SOURCE_DIR}/vSMR/stdafx.cpp
)

# Add shared library (DLL)
add_library(vSMR SHARED ${SOURCES})

# Include directories
target_include_directories(vSMR PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/include
    ${CMAKE_SOURCE_DIR}/vSMR
)

# Link libraries
set(LIBS
    ${CMAKE_SOURCE_DIR}/lib/EuroScopePlugInDll.lib
    winmm.lib
)
target_link_libraries(vSMR PRIVATE 
    ${LIBS}
    asio::asio
    CURL::libcurl
    #ZLIB::ZLIB
    rapidjson
)

# Additional compiler options
target_compile_definitions(vSMR PRIVATE
    WIN32
    _WINDOWS
    NDEBUG
    _USRDLL
    #ASIO_STANDALONE
)

target_compile_options(vSMR PRIVATE /W3 /O2 /MT)

set_target_properties(vSMR PROPERTIES
    MSVC_RUNTIME_LIBRARY "MultiThreaded"
    LINK_FLAGS "/NODEFAULTLIB:msvcrt.lib"
)

# Set output directories
set_target_properties(vSMR PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib
)
