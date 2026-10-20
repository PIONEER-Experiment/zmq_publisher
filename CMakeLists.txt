cmake_minimum_required(VERSION 3.18)
project(zmq_publisher VERSION 0.3.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(ZMQ_PUBLISHER_BUILD_COMMAND_EXTENSION "Build the command processor extension" ON)
option(ZMQ_PUBLISHER_BUILD_MIDAS_EXTENSION "Build the optional MIDAS processor extension" OFF)
option(ZMQ_PUBLISHER_BUILD_GAUDI_EXTENSION "Build the optional MIDAS-to-Gaudi processor extension" OFF)
option(ZMQ_PUBLISHER_BUILD_EXAMPLES "Build repository examples" OFF)
option(ZMQ_PUBLISHER_BUILD_TESTS "Build unit tests" ON)
option(ZMQ_PUBLISHER_BUILD_BENCHMARKS "Build throughput benchmarks" OFF)
option(ZMQ_PUBLISHER_BUILD_RATE_TESTS "Build the MIDAS rate-test frontend" OFF)

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
  OPTIONS "JSON_Install ON"
)

find_package(Threads REQUIRED)
find_library(ZMQ_LIBRARY NAMES zmq REQUIRED)
find_path(CPPZMQ_INCLUDE_DIR zmq.hpp REQUIRED)
set(ZMQ_SYSTEM_LIBRARIES ${ZMQ_LIBRARY})
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Some locally installed libzmq builds use strlcpy from libbsd without
  # propagating it through their CMake/pkg-config metadata.
  unset(BSD_LIBRARY CACHE)
  find_library(BSD_LIBRARY NAMES bsd
    PATHS "/lib/${CMAKE_LIBRARY_ARCHITECTURE}" "/usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}"
    NO_DEFAULT_PATH REQUIRED)
  list(APPEND ZMQ_SYSTEM_LIBRARIES ${BSD_LIBRARY})
endif()

add_library(zmq_publisher_core SHARED
  src/app/application.cpp
  src/channel/channel.cpp
  src/channel/channel_manager.cpp
  src/config/configuration.cpp
  src/config/configuration_view.cpp
  src/extension/plugin_loader.cpp
  src/logging/logger_configuration.cpp
  src/metrics/component_metrics.cpp
  src/metrics/metrics_registry.cpp
  src/metrics/metrics_reporter.cpp
  src/processor/immutable_payload.cpp
  src/processor/output_message.cpp
  src/processor/processor_definition.cpp
  src/processor/process_result.cpp
  src/processor/processor_registry.cpp
  src/scheduler/processor_scheduler.cpp
  src/transport/zmq_transport.cpp
  src/transport/detail/transport_worker.cpp
  src/utility/signal_handler.cpp
)
add_library(zmq_publisher::core ALIAS zmq_publisher_core)
set_target_properties(zmq_publisher_core PROPERTIES EXPORT_NAME core)
add_library(zmq_publisher_plugin_sdk INTERFACE)
add_library(zmq_publisher::plugin_sdk ALIAS zmq_publisher_plugin_sdk)
set_target_properties(zmq_publisher_plugin_sdk PROPERTIES EXPORT_NAME plugin_sdk)
target_include_directories(zmq_publisher_plugin_sdk INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(zmq_publisher_plugin_sdk INTERFACE
  zmq_publisher_core
)
target_include_directories(zmq_publisher_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_include_directories(zmq_publisher_core SYSTEM PRIVATE ${CPPZMQ_INCLUDE_DIR})
target_link_libraries(zmq_publisher_core
  PUBLIC
    $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
    $<BUILD_INTERFACE:spdlog::spdlog>
  PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
    ${ZMQ_SYSTEM_LIBRARIES}
)
target_compile_options(zmq_publisher_core PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

add_executable(zmq_publisher src/app/main.cpp)
target_link_libraries(zmq_publisher PRIVATE
  zmq_publisher::core
  ${ZMQ_SYSTEM_LIBRARIES}
)
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

if(ZMQ_PUBLISHER_BUILD_GAUDI_EXTENSION)
  add_subdirectory(extensions/gaudi)
endif()

if(ZMQ_PUBLISHER_BUILD_EXAMPLES AND EXISTS
   "${CMAKE_CURRENT_SOURCE_DIR}/examples/example_zmq_receiver/CMakeLists.txt")
  add_subdirectory(examples/example_zmq_receiver)
endif()

if(ZMQ_PUBLISHER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ZMQ_PUBLISHER_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(ZMQ_PUBLISHER_BUILD_RATE_TESTS)
  add_subdirectory(benchmarks/rate_tests)
endif()

install(TARGETS zmq_publisher zmq_publisher_core zmq_publisher_plugin_sdk
  EXPORT zmq_publisher_targets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

configure_package_config_file(
  cmake/zmq_publisherConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/zmq_publisherConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zmq_publisher"
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/zmq_publisherConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(EXPORT zmq_publisher_targets
  FILE zmq_publisherTargets.cmake
  NAMESPACE zmq_publisher::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zmq_publisher"
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/zmq_publisherConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/zmq_publisherConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zmq_publisher"
)
