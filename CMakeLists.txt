cmake_minimum_required(VERSION 3.18)
project(publisher LANGUAGES CXX)

# Suppress stringop-overflow warning on GCC globally
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wno-stringop-overflow)
endif()

# ------------------------------------------------------------------------------
# Project Settings
# ------------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ------------------------------------------------------------------------------
# MIDAS Environment (required by many libraries)
# ------------------------------------------------------------------------------
if(NOT DEFINED ENV{MIDASSYS})
  message(FATAL_ERROR
    "Environment variable MIDASSYS not set.\n"
    "Please source your MIDAS environment or run the detect_environment.sh script:\n"
    "  ./scripts/environment/detect_environment.sh\n"
    "  source ./scripts/environment/.env"
  )
endif()

set(MIDASSYS_INCLUDE_DIRS
  $ENV{MIDASSYS}/include
  $ENV{MIDASSYS}/mxml
  $ENV{MIDASSYS}/midasio
)
set(MIDASSYS_LIB_DIR $ENV{MIDASSYS}/lib)

# ------------------------------------------------------------------------------
# CPM Setup (local CPM.cmake expected)
# ------------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPM.cmake)

# ------------------------------------------------------------------------------
# Load CPM Package Config Variables
# ------------------------------------------------------------------------------
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CPMConfig.cmake)

# -------------------------------------------------------------------------
# Automatically add INTERFACE target for header-only packages
# -------------------------------------------------------------------------
function(add_header_only_target pkg)
  if(NOT TARGET ${pkg}_header_only)
    add_library(${pkg}_header_only INTERFACE)
    target_include_directories(${pkg}_header_only INTERFACE
      $<BUILD_INTERFACE:${${pkg}_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:include>
    )
    add_library(${pkg}::${pkg} ALIAS ${pkg}_header_only)
    set(${pkg}_TARGET ${pkg}::${pkg} PARENT_SCOPE)
  endif()
endfunction()

# -------------------------------------------------------------------------
# Loop over all CPM packages
# -------------------------------------------------------------------------
foreach(pkg IN LISTS CPM_PACKAGE_LIST)
  string(TOUPPER ${pkg} UPPER_NAME)

  # Gather options and flags
  set(pkg_OPTIONS "")
  if(DEFINED ${pkg}_OPTIONS)
    set(pkg_OPTIONS ${${pkg}_OPTIONS})
  endif()

  if(DEFINED ${pkg}_DOWNLOAD_ONLY)
    set(pkg_DOWNLOAD_ONLY ${${pkg}_DOWNLOAD_ONLY})
  else()
    set(pkg_DOWNLOAD_ONLY NO)
  endif()

  # Add the package
  CPMAddPackage(
    NAME ${pkg}
    GITHUB_REPOSITORY ${${pkg}_REPO}
    GIT_TAG ${${pkg}_TAG}
    DOWNLOAD_ONLY ${pkg_DOWNLOAD_ONLY}
    OPTIONS ${pkg_OPTIONS}
  )

  # Create interface target for header-only packages
  if(${pkg}_DOWNLOAD_ONLY)
    message(STATUS "Package ${pkg} is header-only or download-only, creating interface target")
    add_header_only_target(${pkg})
  endif()
endforeach()



# ------------------------------------------------------------------------------
# System Dependencies
# ------------------------------------------------------------------------------
find_package(ROOT REQUIRED COMPONENTS Core RIO Tree Hist TreePlayer)
find_package(ZLIB REQUIRED)

find_library(ZMQ_LIBRARY NAMES zmq REQUIRED)
find_path(ZMQ_INCLUDE_DIR zmq.hpp REQUIRED)

# ------------------------------------------------------------------------------
# Sources
# ------------------------------------------------------------------------------
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

add_executable(publisher ${APP_SOURCES})

# ------------------------------------------------------------------------------
# Include Paths
# ------------------------------------------------------------------------------
target_include_directories(publisher PRIVATE
  ${ZMQ_INCLUDE_DIR}
  ${MIDASSYS_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# ------------------------------------------------------------------------------
# Link Libraries
# ------------------------------------------------------------------------------
# Link all registered CPM targets
foreach(pkg IN LISTS CPM_PACKAGE_LIST)
  # Skip linking if target is empty (header-only)
  if(DEFINED ${pkg}_TARGET AND NOT ${${pkg}_TARGET} STREQUAL "")
    target_link_libraries(publisher PRIVATE ${${pkg}_TARGET})
  elseif(DEFINED ${pkg}_TARGETS)
    foreach(subtarget IN LISTS ${pkg}_TARGETS)
      target_link_libraries(publisher PRIVATE ${subtarget})
    endforeach()
  else()
    message(STATUS "Skipping linking header-only or no-target package: ${pkg}")
  endif()
endforeach()


target_link_libraries(publisher PRIVATE
  ROOT::Core ROOT::RIO ROOT::Tree ROOT::Hist ROOT::TreePlayer
  ZLIB::ZLIB
  ${ZMQ_LIBRARY}
  ${MIDASSYS_LIB_DIR}/libmfe.a
  ${MIDASSYS_LIB_DIR}/libmidas.a
)
# ------------------------------------------------------------------------------
# Compiler Definitions (optional)
# ------------------------------------------------------------------------------
target_compile_definitions(publisher PRIVATE
  -DWD2_DONT_INCLUDE_REG_ACCESS_VARS
  -DDCB_DONT_INCLUDE_REG_ACCESS_VARS
)

# ------------------------------------------------------------------------------
# Examples (Submodule)
# ------------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/example_zmq_receiver/CMakeLists.txt")
  message(STATUS "Including example_zmq_receiver submodule for build")
  #add_subdirectory(examples/example_zmq_receiver)
else()
  message(WARNING "example_zmq_receiver submodule NOT found - skipping build")
endif()

# ------------------------------------------------------------------------------
# No install() — this is a top-level application, not a reusable library
# ------------------------------------------------------------------------------
