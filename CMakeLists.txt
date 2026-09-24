cmake_minimum_required(VERSION 3.18)
project(zmq_publisher VERSION 0.2.0 LANGUAGES CXX)

option(ZMQ_PUBLISHER_BUILD_COMMAND_EXTENSION "Build the command processor extension" ON)
option(ZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION "Build the optional MIDAS processor extension" OFF)
option(ZMQ_PUBLISHER_BUILD_EXAMPLES "Build repository examples" OFF)
option(ZMQ_PUBLISHER_BUILD_TESTS "Build unit tests" ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

include(cmake/CPM.cmake)

CPMAddPackage(
  NAME spdlog
  GITHUB_REPOSITORY gabime/spdlog
  GIT_TAG v1.15.3
  OPTIONS "SPDLOG_BUILD_TESTS OFF" "SPDLOG_BUILD_EXAMPLE OFF"
)
CPMAddPackage(
  NAME nlohmann_json
  GITHUB_REPOSITORY nlohmann/json
  GIT_TAG v3.11.3
)

find_package(Threads REQUIRED)
find_library(ZMQ_LIBRARY NAMES zmq REQUIRED)
find_path(CPPZMQ_INCLUDE_DIR zmq.hpp REQUIRED)

add_library(zmq_publisher_core SHARED
  src/app/application.cpp
  src/channel/channel.cpp
  src/channel/channel_manager.cpp
  src/config/configuration.cpp
  src/extension/plugin_loader.cpp
  src/logging/logger_configuration.cpp
  src/processor/processor_registry.cpp
  src/scheduler/processor_scheduler.cpp
  src/transport/zmq_transport.cpp
  src/utility/signal_handler.cpp
)
add_library(zmq_publisher::core ALIAS zmq_publisher_core)
target_include_directories(zmq_publisher_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(zmq_publisher_core SYSTEM PRIVATE ${CPPZMQ_INCLUDE_DIR})
target_link_libraries(zmq_publisher_core
  PUBLIC
    nlohmann_json::nlohmann_json
    spdlog::spdlog
  PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${ZMQ_LIBRARY}
)
target_compile_options(zmq_publisher_core PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

add_executable(zmq_publisher src/app/main.cpp)
target_link_libraries(zmq_publisher PRIVATE zmq_publisher::core)
set_target_properties(zmq_publisher PROPERTIES
  BUILD_RPATH "$ORIGIN/../lib"
  INSTALL_RPATH "$ORIGIN/../lib"
)

if(ZMQ_PUBLISHER_BUILD_COMMAND_EXTENSION)
  add_subdirectory(extensions/command)
endif()

if(ZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION)
  add_subdirectory(extensions/midas)
endif()

if(ZMQ_PUBLISHER_BUILD_EXAMPLES AND EXISTS
   "${CMAKE_CURRENT_SOURCE_DIR}/examples/example_zmq_receiver/CMakeLists.txt")
  add_subdirectory(examples/example_zmq_receiver)
endif()

if(ZMQ_PUBLISHER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

install(TARGETS zmq_publisher zmq_publisher_core
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
