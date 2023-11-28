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

# Add the example_reciever executable
add_executable(example_receiver
   example_receiver/ExampleReceiver.cpp
   utilities/ProjectPrinter.cpp
)


# Check if ZEROMQ_ROOT and CPPZMQ_ROOT are set
if (DEFINED ENV{ZEROMQ_ROOT} AND DEFINED ENV{CPPZMQ_ROOT})
    message(STATUS "ZEROMQ_ROOT and CPPZMQ_ROOT environment variables set.")
    # Define libraries for the "publisher" target
    set(PUBLISHER_LIBS
        # Include ZeroMQ Libraries
        $ENV{ZEROMQ_ROOT}/lib/libzmq.so
    )
    set(RECEIVER_LIBS
        # Include ZeroMQ Libraries
        $ENV{ZEROMQ_ROOT}/lib/libzmq.so
    )
else()
    # Try to find libzmq.so
    find_library(ZMQ_LIBRARY NAMES zmq PATHS "/usr/lib/x86_64-linux-gnu/")
    
    # Try to find zmq.hpp
    find_path(ZMQ_INCLUDE_DIR zmq.hpp PATHS "/usr/include/")
    
    # If both libzmq and zmq.hpp are found, set include and link directories
    if (ZMQ_LIBRARY AND ZMQ_INCLUDE_DIR)
        message(STATUS "Found libzmq.so and zmq.hpp on the system.")
        include_directories(${ZMQ_INCLUDE_DIR})

        # Link the ZeroMQ library
        target_link_libraries(publisher PRIVATE ${ZMQ_LIBRARY})
        target_link_libraries(example_receiver PRIVATE ${ZMQ_LIBRARY})
        
        link_directories("/usr/lib/x86_64-linux-gnu/")
    else()
        # If not found, try to find cppzmq
        find_package(cppzmq QUIET)
        
        # If cppzmq not found, provide hints
        if (NOT cppzmq_FOUND)
            message(WARNING "libzmq.so or zmq.hpp not found. Please set ZEROMQ_ROOT and CPPZMQ_ROOT environment variables manually.")
        endif()
    endif()
endif()




if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
   # Add Linux-specific flags to PUBLISHER_LIBS
   list(APPEND ${PUBLISHER_LIBS} -lpthread -lutil -lrt -ldl)
endif()

target_include_directories(publisher PRIVATE
   # Include ZeroMQ directories
   $ENV{ZEROMQ_ROOT}/include

   # Include cppzmq directories
   $ENV{CPPZMQ_ROOT}/include

   # Include publisher classes
   ${CMAKE_SOURCE_DIR}/data_transmitter
   ${CMAKE_SOURCE_DIR}/utilities
   ${CMAKE_SOURCE_DIR}/command_management
   ${CMAKE_SOURCE_DIR}/processors
)

# Link the "publisher" with libraries including zlib
target_link_libraries(publisher PRIVATE ${PUBLISHER_LIBS})
target_compile_definitions(publisher
   PRIVATE -DWD2_DONT_INCLUDE_REG_ACCESS_VARS
   PRIVATE -DDCB_DONT_INCLUDE_REG_ACCESS_VARS)
set_property(TARGET publisher PROPERTY CXX_STANDARD 17)

# Set the installation directory to the parent directory
set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}") # Install to the parent directory

install(TARGETS publisher DESTINATION ${CMAKE_INSTALL_PREFIX})

#----------------------------------------------------------------------------------

# Include directories for the "example_reciever" target
target_include_directories(example_receiver PRIVATE
   # Include ZeroMQ directories
   $ENV{ZEROMQ_ROOT}/include

   # Include cppzmq directories
   $ENV{CPPZMQ_ROOT}/include

   #Include Utiltiies
   ${CMAKE_SOURCE_DIR}/utilities
)

# Link the "example_reciever" with the specified libraries
target_link_libraries(example_receiver PRIVATE ${RECEIVER_LIBS})


# Set the installation directory for example_receiver
set(RECEIVER_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/example_receiver")
# Set the installation directory for example_reciever
install(TARGETS example_receiver DESTINATION ${RECEIVER_INSTALL_PREFIX})
