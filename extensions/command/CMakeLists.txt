add_library(zmq_publisher_command_extension MODULE
  register_plugin.cpp
  src/command_processor.cpp
  src/command_runner.cpp
)

target_include_directories(zmq_publisher_command_extension
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(zmq_publisher_command_extension PRIVATE zmq_publisher::core)
target_compile_options(zmq_publisher_command_extension PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)
set_target_properties(zmq_publisher_command_extension PROPERTIES
  BUILD_RPATH "$ORIGIN"
  INSTALL_RPATH "$ORIGIN"
)

install(TARGETS zmq_publisher_command_extension LIBRARY DESTINATION lib)
