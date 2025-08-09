cmake_minimum_required(VERSION 3.25)
project(vSMR LANGUAGES CXX RC)

# Ensure MSVC static runtime for a single shipping DLL (no VCRUNTIME deps)
if(MSVC)
  # Use CMake 3.15+ feature
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  # Enable MFC; 2 = use MFC in a Static Library
  set(CMAKE_MFC_FLAG 2)
endif()

# Conan integrations
include(${CMAKE_BINARY_DIR}/conan_deps.cmake OPTIONAL)

# Output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Collect sources
file(GLOB VSMR_SOURCES
    "vSMR/*.cpp"
)
set(VSMR_HEADERS
    vSMR/SMRPlugin.hpp
    vSMR/SMRRadar.hpp
    vSMR/InsetWindow.h
    vSMR/CallsignLookup.hpp
    vSMR/CPDLCSettingsDialog.hpp
    vSMR/DataLinkDialog.hpp
    vSMR/HttpHelper.hpp
    vSMR/Config.hpp
    vSMR/Rimcas.hpp
    vSMR/ColorManager.h
    vSMR/Logger.h
    vSMR/Constant.hpp
    vSMR/stdafx.h
    vSMR/targetver.h
    vSMR/vSMR.hpp
)

set(VSMR_RESOURCES
    vSMR/vSMR.rc
)

add_library(vSMR SHARED ${VSMR_SOURCES} ${VSMR_HEADERS} ${VSMR_RESOURCES})
set_target_properties(vSMR PROPERTIES OUTPUT_NAME "vSMR")

# Precompiled headers
# stdafx.h/cpp are existing PCH; configure MSVC PCH handling
if(MSVC)
  target_precompile_headers(vSMR PRIVATE vSMR/stdafx.h)
endif()

# Definitions needed by MFC
if(MSVC)
  target_compile_definitions(vSMR PRIVATE
    VC_EXTRALEAN
  _MBCS
  _CRT_SECURE_NO_WARNINGS
  ASIO_STANDALONE
  CURL_STATICLIB
  )
endif()

# Include EuroScope SDK headers bundled in repo
target_include_directories(vSMR PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vSMR
)

# Link EuroScope SDK import library (32-bit). Provided under lib/EuroScopePlugInDll.lib
# Also link required Windows libs
find_package(CURL REQUIRED)
find_package(asio REQUIRED CONFIG)
find_package(RapidJSON REQUIRED CONFIG)

target_link_libraries(vSMR
  PRIVATE
  CURL::libcurl
  rapidjson
  asio::asio
)

# Windows and MFC libraries
if(MSVC)
  target_link_libraries(vSMR PRIVATE
    version
    winmm
  Gdiplus
    comdlg32
    gdi32
    user32
    advapi32
    shell32
    ole32
    oleaut32
    uuid
    Ws2_32
    Crypt32
  )
  # Link the EuroScope import lib from repo
  target_link_libraries(vSMR PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/EuroScopePlugInDll.lib
  )
  # Ensure /DELAYLOAD:version if needed; usually not required.
endif()

# Ensure we compile as 32-bit when requested
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(WARNING "Building 64-bit; EuroScope is 32-bit. Configure with -A Win32 or Conan -s arch=x86.")
endif()

# Resource include directories
set_source_files_properties(${VSMR_RESOURCES} PROPERTIES LANGUAGE RC)
if(MSVC)
  # Add paths used by .rc for local resource includes
  target_compile_options(vSMR PRIVATE
    $<$<COMPILE_LANGUAGE:RC>:/I${CMAKE_CURRENT_SOURCE_DIR}/vSMR>
    $<$<COMPILE_LANGUAGE:RC>:/I${CMAKE_CURRENT_SOURCE_DIR}>
  )
endif()

if(MSVC)
  target_compile_options(vSMR PRIVATE /W3)
  # Ensure _AFXDLL is not defined
  target_compile_options(vSMR PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/U_AFXDLL>)
endif()
