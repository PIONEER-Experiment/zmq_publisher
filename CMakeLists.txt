cmake_minimum_required(VERSION 3.0)
project(publisher)

# Recursively find all .cpp source files under src/
file(GLOB_RECURSE ALL_SRC_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

# Filter out example_receiver sources for publisher
set(SRC_SOURCES "")
foreach(src_file ${ALL_SRC_SOURCES})
    if(NOT src_file MATCHES "/example_receiver/" AND NOT src_file MATCHES "/build/")
        list(APPEND SRC_SOURCES ${src_file})
    endif()
endforeach()

# Recursively find all .h header files under include/
file(GLOB_RECURSE HEADER_FILES CONFIGURE_DEPENDS "include/*.h")

# Recursively collect all include subdirectories inside include/
file(GLOB_RECURSE ALL_INCLUDE_DIRS LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/include/*")

# Filter only directories (remove files)
set(FILTERED_INCLUDE_DIRS "")
foreach(dir ${ALL_INCLUDE_DIRS})
    if(IS_DIRECTORY ${dir})
        list(APPEND FILTERED_INCLUDE_DIRS ${dir})
    endif()
endforeach()

# Create output directory for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Publisher executable using filtered source files
add_executable(publisher ${SRC_SOURCES})

# Example receiver executable sources (unchanged)
add_executable(example_receiver
    src/example_receiver/ExampleReceiver.cpp
    src/utilities/ProjectPrinter.cpp
)

# Add the filtered include directories to both targets
target_include_directories(publisher PRIVATE ${FILTERED_INCLUDE_DIRS})
target_include_directories(example_receiver PRIVATE ${FILTERED_INCLUDE_DIRS})

# ZeroMQ and cppzmq detection
set(PUBLISHER_LIBS "")
set(RECEIVER_LIBS "")

if (DEFINED ENV{ZEROMQ_ROOT} AND DEFINED ENV{CPPZMQ_ROOT})
    # Using ZeroMQ and cppzmq from environment variables

    target_include_directories(publisher PRIVATE
        $ENV{ZEROMQ_ROOT}/include
        $ENV{CPPZMQ_ROOT}/include
    )

    target_include_directories(example_receiver PRIVATE
        $ENV{ZEROMQ_ROOT}/include
        $ENV{CPPZMQ_ROOT}/include
    )

    list(APPEND PUBLISHER_LIBS "$ENV{ZEROMQ_ROOT}/build//lib/libzmq.so")
    list(APPEND RECEIVER_LIBS "$ENV{ZEROMQ_ROOT}/build/lib/libzmq.so")

else()
    find_library(ZMQ_LIBRARY NAMES zmq)
    find_path(ZMQ_INCLUDE_DIR zmq.hpp)

    find_package(cppzmq QUIET)

    if(ZMQ_LIBRARY AND ZMQ_INCLUDE_DIR)

        target_include_directories(publisher PRIVATE ${ZMQ_INCLUDE_DIR})
        target_include_directories(example_receiver PRIVATE ${ZMQ_INCLUDE_DIR})

        list(APPEND PUBLISHER_LIBS ${ZMQ_LIBRARY})
        list(APPEND RECEIVER_LIBS ${ZMQ_LIBRARY})

        if(cppzmq_FOUND)
            target_link_libraries(publisher PRIVATE cppzmq)
            target_link_libraries(example_receiver PRIVATE cppzmq)
        endif()
    else()
        message(FATAL_ERROR "Could not find ZeroMQ library or zmq.hpp. Please install ZeroMQ and cppzmq or set ZEROMQ_ROOT and CPPZMQ_ROOT environment variables.")
    endif()
endif()

# MIDAS support
set(MIDAS_RECEIVER_DIR "/home/jack/cpp_projects/midas_stuff/midas_receiver")

set(MIDAS_LIBS
    $ENV{MIDASSYS}/lib/libmfe.a
    $ENV{MIDASSYS}/lib/libmidas.a
)

target_include_directories(publisher PRIVATE
    ${MIDAS_RECEIVER_DIR}/include
    $ENV{MIDASSYS}/include
    $ENV{MIDASSYS}/mxml
    $ENV{MIDASSYS}/midasio
)

list(APPEND PUBLISHER_LIBS
    ${MIDAS_RECEIVER_DIR}/lib/libmidas_receiver_lib.a
    ${MIDAS_LIBS}
)

find_package(ZLIB REQUIRED)
list(APPEND PUBLISHER_LIBS ZLIB::ZLIB)

if(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    list(APPEND PUBLISHER_LIBS -lpthread -lutil -lrt -ldl)
endif()

target_link_libraries(publisher PRIVATE ${PUBLISHER_LIBS})
target_link_libraries(example_receiver PRIVATE ${RECEIVER_LIBS})

target_compile_definitions(publisher PRIVATE -DWD2_DONT_INCLUDE_REG_ACCESS_VARS -DDCB_DONT_INCLUDE_REG_ACCESS_VARS)
set_property(TARGET publisher PROPERTY CXX_STANDARD 17)

# Installation - bin folder inside source dir
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}")

install(TARGETS publisher example_receiver
    RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)
