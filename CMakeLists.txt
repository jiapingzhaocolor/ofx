cmake_minimum_required(VERSION 3.16)
project(SplitToneV2_OFX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenFX Support in repo (submodule)
set(OFX_SUPPORT_ROOT "${CMAKE_SOURCE_DIR}/external/openfx/Support")
include_directories("${OFX_SUPPORT_ROOT}/include")

# Compile the Support wrapper sources directly into the plugin
file(GLOB OFX_SUPPORT_SOURCES
  "${OFX_SUPPORT_ROOT}/Library/*.cpp"
  "${OFX_SUPPORT_ROOT}/Library/*.c"
)

add_library(SplitToneV2 MODULE
  SplitTone_v2.cpp
  ${OFX_SUPPORT_SOURCES}
)

# IMPORTANT: no "lib" prefix, and plugin binary must end with ".ofx"
set_target_properties(SplitToneV2 PROPERTIES PREFIX "" SUFFIX ".ofx")
