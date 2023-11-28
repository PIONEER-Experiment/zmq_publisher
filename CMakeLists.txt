cmake_minimum_required(VERSION 3.0)

project(publisher)

if (NOT DEFINED ENV{ZEROMQ_ROOT})
   message(SEND_ERROR "ZEROMQ_ROOT environment variable not defined.")
endif()
if (NOT DEFINED ENV{CPPZMQ_ROOT})
   message(SEND_ERROR "CPPZMQ_ROOT environment variable not defined.")
endif()

# Find and use the zlib library
find_package(ZLIB REQUIRED)

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


# Define libraries for the "publisher" target
set(PUBLISHER_LIBS
   # Include ZeroMQ Libraries
   $ENV{ZEROMQ_ROOT}/lib/libzmq.so
)

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
   # Add Linux-specific flags to PUBLISHER_LIBS
   list(APPEND PUBLISHER_LIBS -lpthread -lutil -lrt -ldl)
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

# Add the example_reciever executable
add_executable(example_receiver
   example_receiver/ExampleReceiver.cpp
   utilities/ProjectPrinter.cpp
)

# Define libraries for the "example_reciever" target
set(RECEIVER_LIBS
   # Include ZeroMQ Libraries
   $ENV{ZEROMQ_ROOT}/lib/libzmq.so
)

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
