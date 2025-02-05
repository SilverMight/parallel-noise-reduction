cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    parallel-noise-reduction
    VERSION 0.1.0
    DESCRIPTION "Parallel noise reduction"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(
    parallel-noise-reduction_lib OBJECT
    source/lib.cpp
)

target_include_directories(
    parallel-noise-reduction_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/source>"
)

target_compile_features(parallel-noise-reduction_lib PUBLIC cxx_std_20)

find_package(fmt REQUIRED)
target_link_libraries(parallel-noise-reduction_lib PRIVATE fmt::fmt)

# ---- Declare executable ----

add_executable(parallel-noise-reduction_exe source/main.cpp)
add_executable(parallel-noise-reduction::exe ALIAS parallel-noise-reduction_exe)

set_property(TARGET parallel-noise-reduction_exe PROPERTY OUTPUT_NAME parallel-noise-reduction)

target_compile_features(parallel-noise-reduction_exe PRIVATE cxx_std_20)

target_link_libraries(parallel-noise-reduction_exe PRIVATE parallel-noise-reduction_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT parallel-noise-reduction_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of parallel-noise-reduction"
  )
endif()

include(cmake/dev-mode.cmake)
