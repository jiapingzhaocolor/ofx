cmake_minimum_required(VERSION 3.16)
project(SplitToneV2_OFX LANGUAGES C CXX)

# This project expects the OpenFX "Support" library (C++ wrappers) to be available.
# Set OFX_SUPPORT_ROOT to the folder containing:
#   include/ofxsImageEffect.h
#   include/ofxsProcessing.H
#   lib/ (or build output)
#
# Common layout: https://github.com/AcademySoftwareFoundation/openfx/tree/master/Support
# (Bring it in as a submodule, or point OFX_SUPPORT_ROOT to your local copy.)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT OFX_SUPPORT_ROOT)
  message(FATAL_ERROR "Please set -DOFX_SUPPORT_ROOT=/path/to/openfx/Support")
endif()

set(OFX_SUPPORT_INCLUDE "${OFX_SUPPORT_ROOT}/include")
include_directories("${OFX_SUPPORT_INCLUDE}")

# If you built Support as a library, point this to your build output and adjust name.
# Many people just compile the needed Support sources directly; keep it simple here.
#
# Minimal approach: compile the Support 'library' sources as part of this target.
# You'll need to add the relevant Support .cpp files from your Support checkout:
#   ofxsCore.cpp, ofxsImageEffect.cpp, ofxsMultiThread.cpp, ofxsPropertySuite.cpp, etc.
#
# Because Support layouts vary between versions, we keep this CMake minimal.

add_library(SplitToneV2 SHARED SplitTone_v2.cpp)

# On macOS, OFX bundles are usually .ofx.bundle. On Linux/Windows, shared lib.
# You will likely need to set:
#   -DOfxSupport_DIR=... or add Support sources to this target
# and link against the host's required system libs.

if(APPLE)
  set_target_properties(SplitToneV2 PROPERTIES
    OUTPUT_NAME "SplitToneV2"
    SUFFIX ".ofx"
  )
endif()
