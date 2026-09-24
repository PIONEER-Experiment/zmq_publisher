if(NOT DEFINED ENV{MIDASSYS})
  message(FATAL_ERROR "MIDASSYS is required when ZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION=ON")
endif()

CPMAddPackage(
  NAME midas_receiver
  GITHUB_REPOSITORY jaca230/midas_receiver
  GIT_TAG main
  OPTIONS
    "CMAKE_POSITION_INDEPENDENT_CODE ON"
    "BUILD_MIDAS_RECEIVER_TEST OFF"
)
find_package(ZLIB REQUIRED)

add_library(zmq_publisher_midas_extension MODULE
  register_plugin.cpp
  src/midas_processor.cpp
)

target_include_directories(zmq_publisher_midas_extension
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    $ENV{MIDASSYS}/include
    $ENV{MIDASSYS}/mxml
    $ENV{MIDASSYS}/midasio
)
target_link_libraries(zmq_publisher_midas_extension PRIVATE
  zmq_publisher::core
  MidasReceiver::midas_receiver
  ZLIB::ZLIB
  $ENV{MIDASSYS}/lib/libmfe.a
  $ENV{MIDASSYS}/lib/libmidas.a
)
target_compile_definitions(zmq_publisher_midas_extension PRIVATE
  WD2_DONT_INCLUDE_REG_ACCESS_VARS
  DCB_DONT_INCLUDE_REG_ACCESS_VARS
)
target_compile_options(zmq_publisher_midas_extension PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)
set_target_properties(zmq_publisher_midas_extension PROPERTIES
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
)

install(TARGETS zmq_publisher_midas_extension LIBRARY DESTINATION lib)
