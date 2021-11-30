cmake_minimum_required (VERSION 3.0)
project (SynthMakr)

set (CMAKE_CXX_STANDARD 20)

add_subdirectory(libs)

set(SRC "${SynthMakr_SOURCE_DIR}/")

file(GLOB_RECURSE SOURCE
  "${SRC}source/*.cpp"
  "${SRC}include/*.hpp"
)

add_executable(SynthMakr
  ${SOURCE}
)

target_include_directories(SynthMakr PUBLIC
  libs/GuiCode2/include
  libs/GuiCode2/libs
  ${AUDIJO_INCLUDE_DIRS}
  ${MIDIJO_INCLUDE_DIRS}
  include/
)

source_group(TREE ${SRC} FILES ${SOURCE})

target_precompile_headers(SynthMakr PUBLIC
  "${SRC}include/pch.hpp"
)

target_link_libraries(SynthMakr
  GuiCode2
  Audijo
  Midijo
)