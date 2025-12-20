CMAKE_MINIMUM_REQUIRED(VERSION 3.20)

PROJECT(vSMR 
    VERSION 1.0.0
    LANGUAGES CXX
)

# Global project settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optimization settings
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)

# Find dependencies
find_package(RapidJSON REQUIRED)

# Configure version header
configure_file(
    ${CMAKE_SOURCE_DIR}/inc/Version.h.inc
    ${CMAKE_CURRENT_BINARY_DIR}/Version.h
    @ONLY
)

# ==============================================================================
# EuroScope Plugin Interface Library (Header-only)
# ==============================================================================
add_library(euroscope_interface INTERFACE)
target_include_directories(euroscope_interface INTERFACE
    ${CMAKE_SOURCE_DIR}/euroscope/inc
)

# ==============================================================================
# Module Libraries
# ==============================================================================
add_subdirectory(src/Logger)
add_subdirectory(src/Radar)
#add_subdirectory(src/ACARS)
#add_subdirectory(src/RIMCAS)
#add_subdirectory(src/Tags)
add_subdirectory(src/Utils)

# ==============================================================================
# Main Plugin Target
# ==============================================================================
add_library(vSMR SHARED
    src/main.cpp
    src/vSMRPlugIn.cpp
)

# Include directories
target_include_directories(vSMR
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_BINARY_DIR}
)

# Compile definitions
target_compile_definitions(vSMR
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        NOMINMAX
        $<$<CONFIG:Debug>:DEBUG_BUILD=1>
)

# Compile options
if(MSVC)
    target_compile_options(vSMR
        PRIVATE
            /W4          # Warning level 4
            /MP          # Multi-processor compilation
            /sdl         # Enable additional security checks
            /permissive- # Standards conformance mode
            /EHsc        # Enable C++ exception handling
    )
    
    target_link_options(vSMR
        PRIVATE
            /MANIFESTUAC:NO
            /ignore:4099
    )
endif()

# Link libraries
target_link_libraries(vSMR
    PRIVATE
        euroscope_interface
        rapidjson
        vSMR_Logger
        vSMR_Radar
        #vSMR_ACARS
        #vSMR_RIMCAS
        #vSMR_Tags
        vSMR_Utils
        ${CMAKE_SOURCE_DIR}/euroscope/lib/EuroScopePlugInDLL.lib
        crypt32
        ws2_32
        Shlwapi
)

# Set output properties
set_target_properties(vSMR PROPERTIES
    OUTPUT_NAME "vSMR"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)