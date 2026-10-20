if(NOT DEFINED ENV{MIDASSYS})
  message(FATAL_ERROR "MIDASSYS is required when ZMQ_PUBLISHER_BUILD_GAUDI_EXTENSION=ON")
endif()

if(NOT TARGET MidasReceiver::midas_receiver)
  CPMAddPackage(
    NAME midas_receiver
    GITHUB_REPOSITORY jaca230/midas_receiver
    GIT_TAG main
    OPTIONS
      "CMAKE_POSITION_INDEPENDENT_CODE ON"
      "BUILD_MIDAS_RECEIVER_TEST OFF"
  )
endif()
find_package(ZLIB REQUIRED)

add_library(zmq_publisher_gaudi_client STATIC
  src/gaudi_client.cpp
)
target_include_directories(zmq_publisher_gaudi_client PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(zmq_publisher_gaudi_client PUBLIC zmq_publisher::core)
target_compile_options(zmq_publisher_gaudi_client PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

add_library(zmq_publisher_gaudi_extension MODULE
  register_plugin.cpp
  src/gaudi_processor.cpp
  src/gaudi_processor_config.cpp
  src/sidecar_process.cpp
)

target_include_directories(zmq_publisher_gaudi_extension
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    $ENV{MIDASSYS}/include
    $ENV{MIDASSYS}/mxml
    $ENV{MIDASSYS}/midasio
)
target_link_libraries(zmq_publisher_gaudi_extension PRIVATE
  zmq_publisher::core
  zmq_publisher_gaudi_client
  MidasReceiver::midas_receiver
  ZLIB::ZLIB
  $ENV{MIDASSYS}/lib/libmfe.a
  $ENV{MIDASSYS}/lib/libmidas.a
)
target_compile_definitions(zmq_publisher_gaudi_extension PRIVATE
  WD2_DONT_INCLUDE_REG_ACCESS_VARS
  DCB_DONT_INCLUDE_REG_ACCESS_VARS
)
target_compile_options(zmq_publisher_gaudi_extension PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)
set_target_properties(zmq_publisher_gaudi_extension PROPERTIES
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
)

install(TARGETS zmq_publisher_gaudi_extension LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
