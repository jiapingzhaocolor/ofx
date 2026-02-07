cmake_minimum_required(VERSION 3.16)
project(SplitToneV2_OFX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GitHub Actions will pass:
# -DOFX_SUPPORT_ROOT="${{ github.workspace }}/external/openfx/Support"
set(OFX_SUPPORT_ROOT "" CACHE PATH "Path to OpenFX Support directory")
if(NOT OFX_SUPPORT_ROOT)
  message(FATAL_ERROR "Set -DOFX_SUPPORT_ROOT=/path/to/openfx/Support")
endif()

# OpenFX root is parent of Support/
get_filename_component(OFX_ROOT "${OFX_SUPPORT_ROOT}" DIRECTORY)

message(STATUS "OFX_ROOT         = ${OFX_ROOT}")
message(STATUS "OFX_SUPPORT_ROOT = ${OFX_SUPPORT_ROOT}")
message(STATUS "Checking header  = ${OFX_SUPPORT_ROOT}/Plugins/include/ofxsProcessing.H")

# Hard fail if the expected header isn't present (catches path mistakes immediately)
if(NOT EXISTS "${OFX_SUPPORT_ROOT}/Plugins/include/ofxsProcessing.H")
  message(FATAL_ERROR "Missing header: ${OFX_SUPPORT_ROOT}/Plugins/include/ofxsProcessing.H (OFX_SUPPORT_ROOT is probably wrong)")
endif()

# Compile OpenFX Support wrapper sources directly into the plugin
file(GLOB OFX_SUPPORT_SOURCES
  "${OFX_SUPPORT_ROOT}/Library/*.cpp"
  "${OFX_SUPPORT_ROOT}/Library/*.c"
)

add_library(SplitToneV2 MODULE
  SplitTone_v2.cpp
  ${OFX_SUPPORT_SOURCES}
)

# CRITICAL naming: output must be SplitToneV2.ofx (no lib prefix)
set_target_properties(SplitToneV2 PROPERTIES
  PREFIX ""
  SUFFIX ".ofx"
)

# Include OpenFX core headers + Support headers + Support Plugins headers (for ofxsProcessing.H)
target_include_directories(SplitToneV2 PRIVATE
  "${OFX_ROOT}/include"
  "${OFX_SUPPORT_ROOT}/include"
  "${OFX_SUPPORT_ROOT}/Plugins/include"
)

# Linux commonly needs these
if(UNIX AND NOT APPLE)
  target_link_libraries(SplitToneV2 PRIVATE dl pthread)
endif()
