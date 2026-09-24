CPMAddPackage(
  NAME Catch2
  GITHUB_REPOSITORY catchorg/Catch2
  GIT_TAG v3.15.3
  OPTIONS
    "CATCH_INSTALL_DOCS OFF"
    "CATCH_INSTALL_EXTRAS OFF"
)

set(ZMQ_PUBLISHER_TEST_SOURCES
  channel_test.cpp
  data_buffer_test.cpp
  processor_registry_test.cpp
  processor_scheduler_test.cpp
)

if(TARGET zmq_publisher_command_extension)
  list(APPEND ZMQ_PUBLISHER_TEST_SOURCES command_extension_test.cpp)
endif()

add_executable(zmq_publisher_tests ${ZMQ_PUBLISHER_TEST_SOURCES})
target_link_libraries(zmq_publisher_tests PRIVATE
  Catch2::Catch2WithMain
  zmq_publisher::core
)

if(TARGET zmq_publisher_command_extension)
  add_dependencies(zmq_publisher_tests zmq_publisher_command_extension)
  target_compile_definitions(zmq_publisher_tests PRIVATE
    ZMQ_PUBLISHER_COMMAND_EXTENSION_PATH="$<TARGET_FILE:zmq_publisher_command_extension>"
  )
endif()

include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(zmq_publisher_tests)
