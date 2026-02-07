cmake_minimum_required(VERSION 3.16)
project(SplitToneV2_OFX LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OFX_SUPPORT_ROOT "" CACHE PATH "Path to OpenFX Support directory")
if(NOT OFX_SUPPORT_ROOT)
  message(FATAL_ERROR "Set -DOFX_SUPPORT_ROOT=/path/to/openfx/Support")
endif()

get_filename_component(OFX_ROOT "${OFX_SUPPORT_ROOT}" DIRECTORY)

include_directories(
  "${OFX_ROOT}/include"
  "${OFX_SUPPORT_ROOT}/include"
)

file(GLOB OFX_SUPPORT_SOURCES
  "${OFX_SUPPORT_ROOT}/Library/*.cpp"
  "${OFX_SUPPORT_ROOT}/Library/*.c"
)

add_library(SplitToneV2 MODULE
  SplitTone_v2.cpp
  ${OFX_SUPPORT_SOURCES}
)

set_target_properties(SplitToneV2 PROPERTIES PREFIX "" SUFFIX ".ofx")

if(UNIX AND NOT APPLE)
  target_link_libraries(SplitToneV2 PRIVATE dl pthread)
endif()
