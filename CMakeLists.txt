cmake_minimum_required(VERSION 3.0)

project(publisher)

file(GLOB DATA_TRANSMITTER_SOURCES "data_transmitter/*.cpp")
file(GLOB COMMAND_MANAGEMENT_SOURCES "command_management/*.cpp")
file(GLOB PROCESSORS_SOURCES "processors/*.cpp")
file(GLOB UTILITIES_SOURCES "utilities/*.cpp")

# Create the publisher executable
add_executable(publisher
    ${DATA_TRANSMITTER_SOURCES}
    ${COMMAND_MANAGEMENT_SOURCES}
    ${PROCESSORS_SOURCES}
    ${UTILITIES_SOURCES}
    main.cpp
)

# Create the example_receiver executable
add_executable(example_receiver
    example_receiver/ExampleReceiver.cpp
    utilities/ProjectPrinter.cpp
)

# Variables to hold libraries to link
set(PUBLISHER_LIBS "")
set(RECEIVER_LIBS "")

# Handle ZeroMQ and cppzmq
if (DEFINED ENV{ZEROMQ_ROOT} AND DEFINED ENV{CPPZMQ_ROOT})
    message(STATUS "Using ZeroMQ and cppzmq from environment variables")

    target_include_directories(publisher PRIVATE
        $ENV{ZEROMQ_ROOT}/include
        $ENV{CPPZMQ_ROOT}/include
        ${CMAKE_SOURCE_DIR}/data_transmitter
        ${CMAKE_SOURCE_DIR}/utilities
        ${CMAKE_SOURCE_DIR}/command_management
        ${CMAKE_SOURCE_DIR}/processors
    )

    target_include_directories(example_receiver PRIVATE
        $ENV{ZEROMQ_ROOT}/include
        $ENV{CPPZMQ_ROOT}/include
        ${CMAKE_SOURCE_DIR}/utilities
    )

    list(APPEND PUBLISHER_LIBS "$ENV{ZEROMQ_ROOT}/lib/libzmq.so")
    list(APPEND RECEIVER_LIBS "$ENV{ZEROMQ_ROOT}/lib/libzmq.so")

else()
    find_library(ZMQ_LIBRARY NAMES zmq)
    find_path(ZMQ_INCLUDE_DIR zmq.hpp)

    find_package(cppzmq QUIET)

    if(ZMQ_LIBRARY AND ZMQ_INCLUDE_DIR)
        message(STATUS "Found ZeroMQ library and zmq.hpp")

        target_include_directories(publisher PRIVATE
            ${ZMQ_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}/data_transmitter
            ${CMAKE_SOURCE_DIR}/utilities
            ${CMAKE_SOURCE_DIR}/command_management
            ${CMAKE_SOURCE_DIR}/processors
        )

        target_include_directories(example_receiver PRIVATE
            ${ZMQ_INCLUDE_DIR}
            ${CMAKE_SOURCE_DIR}/utilities
        )

        list(APPEND PUBLISHER_LIBS ${ZMQ_LIBRARY})
        list(APPEND RECEIVER_LIBS ${ZMQ_LIBRARY})

        if(cppzmq_FOUND)
            message(STATUS "Found cppzmq package, linking it")
            target_link_libraries(publisher PRIVATE cppzmq)
            target_link_libraries(example_receiver PRIVATE cppzmq)
        else()
            message(WARNING "cppzmq package not found, continuing without linking cppzmq")
        endif()
    else()
        message(FATAL_ERROR "Could not find ZeroMQ library or zmq.hpp. Please install ZeroMQ and cppzmq or set ZEROMQ_ROOT and CPPZMQ_ROOT environment variables.")
    endif()
endif()

# === MIDAS support ===
set(MIDAS_RECEIVER_DIR "/home/jack/cpp_projects/midas_stuff/midas_receiver")

# MIDAS libraries that your midas_receiver depends on
set(MIDAS_LIBS
    $ENV{MIDASSYS}/lib/libmfe.a
    $ENV{MIDASSYS}/lib/libmidas.a
)

# Include directories for MIDAS and midas_receiver
target_include_directories(publisher PRIVATE
    ${MIDAS_RECEIVER_DIR}/include
    $ENV{MIDASSYS}/include
    $ENV{MIDASSYS}/mxml
    $ENV{MIDASSYS}/midasio
)

# Link midas_receiver lib + its dependencies + zlib
find_package(ZLIB REQUIRED)
list(APPEND PUBLISHER_LIBS
    ${MIDAS_RECEIVER_DIR}/lib/libmidas_receiver_lib.a
    ${MIDAS_LIBS}
    ZLIB::ZLIB
)

# Linux-specific system libraries
if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    list(APPEND PUBLISHER_LIBS -lpthread -lutil -lrt -ldl)
endif()

# Link libraries to targets
target_link_libraries(publisher PRIVATE ${PUBLISHER_LIBS})
target_link_libraries(example_receiver PRIVATE ${RECEIVER_LIBS})

# Compile options and standards
target_compile_definitions(publisher PRIVATE -DWD2_DONT_INCLUDE_REG_ACCESS_VARS -DDCB_DONT_INCLUDE_REG_ACCESS_VARS)
set_property(TARGET publisher PROPERTY CXX_STANDARD 17)

# Installation paths
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")

install(TARGETS publisher DESTINATION ${CMAKE_INSTALL_PREFIX})

set(RECEIVER_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/example_receiver")
install(TARGETS example_receiver DESTINATION ${RECEIVER_INSTALL_PREFIX})
