cmake_minimum_required(VERSION 3.16)
project(SplitToneV2_OFX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# GitHub Actions passes this in:
# -DOFX_SUPPORT_ROOT="${{ github.workspace }}/external/openfx/Support"
set(OFX_SUPPORT_ROOT "" CACHE PATH "Path to OpenFX Support directory")
if(NOT OFX_SUPPORT_ROOT)
  message(FATAL_ERROR "Set -DOFX_SUPPORT_ROOT=/path/to/openfx/Support")
endif()

# OpenFX root is the parent of Support/
get_filename_component(OFX_ROOT "${OFX_SUPPORT_ROOT}" DIRECTORY)

# IMPORTANT: include all required header locations
include_directories(
  "${OFX_ROOT}/include"                  # ofxCore.h, ofxImageEffect.h, etc.
  "${OFX_SUPPORT_ROOT}/include"          # ofxs*.h
  "${OFX_SUPPORT_ROOT}/Plugins/include"  # ofxsProcessing.H
)

# Compile OpenFX Support wrapper sources directly into the plugin
file(GLOB OFX_SUPPORT_SOURCES
  "${OFX_SUPPORT_ROOT}/Library/*.cpp"
  "${OFX_SUPPORT_ROOT}/Library/*.c"
)

# Build the OFX plugin binary
add_library(SplitToneV2 MODULE
  SplitTone_v2.cpp
  ${OFX_SUPPORT_SOURCES}
)

# CRITICAL naming: no "lib" prefix, and output must end with ".ofx"
set_target_properties(SplitToneV2 PROPERTIES
  PREFIX ""
  SUFFIX ".ofx"
)

# Linux commonly needs these
if(UNIX AND NOT APPLE)
  target_link_libraries(SplitToneV2 PRIVATE dl pthread)
endif()
