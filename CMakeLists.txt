cmake_minimum_required(VERSION 3.0)

project(publisher)

if (NOT DEFINED ENV{MIDASSYS})
   message(SEND_ERROR "MIDASSYS environment variable not defined.")
endif()
if (NOT DEFINED ENV{UNPACKING_ROOT})
   message(SEND_ERROR "UNPACKING_ROOT environment variable not defined.")
endif()
if (NOT DEFINED ENV{ZEROMQ_ROOT})
   message(SEND_ERROR "ZEROMQ_ROOT environment variable not defined.")
endif()
if (NOT DEFINED ENV{CPPZMQ_ROOT})
   message(SEND_ERROR "CPPZMQ_ROOT environment variable not defined.")
endif()
if (NOT DEFINED ENV{BOOST_1_70_0_ROOT})
   message(SEND_ERROR "BOOST_1_70_0_ROOT environment variable not defined.")
endif()
if (NOT DEFINED ENV{EIGEN_ROOT})
   message(SEND_ERROR "EIGEN_ROOT environment variable not defined.")
endif()


if(DEFINED ENV{BOOST_1_70_0_ROOT})
    # Append the BOOST_1_70_0_ROOT to CMAKE_PREFIX_PATH
    list(APPEND CMAKE_PREFIX_PATH "$ENV{BOOST_1_70_0_ROOT}")
endif()
find_package(Boost 1.70 REQUIRED COMPONENTS system filesystem)

find_package(ROOT REQUIRED)
include(${ROOT_USE_FILE})
include("${ROOT_DIR}/RootMacros.cmake")

# Find and use the zlib library
find_package(ZLIB REQUIRED)

# Create the publisher executable
add_executable(publisher)

target_sources(publisher
   PRIVATE
   data_transmitter/DataTransmitter.cpp
   data_transmitter/DataTransmitterManager.cpp
   data_transmitter/DataChannel.cpp
   data_transmitter/DataChannelManager.cpp
   data_transmitter/DataChannelProcessesManager.cpp
   event_processor/EventProcessor.cpp
   event_processor/MidasBank.cpp
   event_processor/MidasEvent.cpp
   event_processor/MdumpPackage.cpp
   midas_connector/MidasConnector.cpp
   midas_connector/ODBGrabber.cpp
   command_management/CommandRunner.cpp
   command_management/MdumpCommandRunner.cpp
   processors/GeneralProcessor.cpp
   processors/GeneralProcessorFactory.cpp
   processors/CommandProcessor.cpp
   processors/MdumpProcessor.cpp
   processors/ODBProcessor.cpp
   processors/HistogramProcessor.cpp
   processors/RunNumberProcessor.cpp
   histogram_management/HistogramStorage.cpp
   multi_threading/MdumpDatabase.cpp
   utilities/ProjectPrinter.cpp
   utilities/EventLoopManager.cpp
   utilities/SignalHandler.cpp
   utilities/JsonManager.cpp
   main.cpp
)

# Define libraries for the "publisher" target
set(PUBLISHER_LIBS
   # Midas libs
   $ENV{MIDASSYS}/lib/libmfe.a
   $ENV{MIDASSYS}/lib/libmidas.a

   # ROOT libs
   $ENV{ROOTSYS}/lib/libCore.so
   ${ROOT_LIBRARIES}

   # Include unpacking libraries
   $ENV{UNPACKING_ROOT}/lib/libdataProducts.so
   $ENV{UNPACKING_ROOT}/lib/libmidas.so
   $ENV{UNPACKING_ROOT}/lib/libunpackers.so
   $ENV{UNPACKING_ROOT}/lib/libserializer.so
   $ENV{UNPACKING_ROOT}/lib/libserializer.so

   # Include ZeroMQ Libraries
   $ENV{ZEROMQ_ROOT}/lib/libzmq.so

   # Include Boost Libraries
   ${Boost_LIBRARIES}

   # Include ZLIB::ZLIB
   ZLIB::ZLIB
)

if (${CMAKE_SYSTEM_NAME} MATCHES Linux)
   # Add Linux-specific flags to PUBLISHER_LIBS
   list(APPEND PUBLISHER_LIBS -lpthread -lutil -lrt -ldl)
endif()

target_include_directories(publisher PRIVATE
   # Include Root directories
   ${ROOT_INCLUDE_DIRS}

   # Include Midas directories
   $ENV{MIDASSYS}/include
   $ENV{MIDASSYS}/mxml
   $ENV{MIDASSYS}/midasio

   # Include Unpacking directories
   $ENV{UNPACKING_ROOT}/src
   $ENV{UNPACKING_ROOT}/external

   # Include ZeroMQ directories
   $ENV{ZEROMQ_ROOT}/include

   # Include cppzmq directories
   $ENV{CPPZMQ_ROOT}/include

   # Include publisher classes
   ${CMAKE_SOURCE_DIR}/data_transmitter
   ${CMAKE_SOURCE_DIR}/event_processor
   ${CMAKE_SOURCE_DIR}/midas_connector
   ${CMAKE_SOURCE_DIR}/utilities
   ${CMAKE_SOURCE_DIR}/command_management
   ${CMAKE_SOURCE_DIR}/processors
   ${CMAKE_SOURCE_DIR}/histogram_management
   ${CMAKE_SOURCE_DIR}/multi_threading

   # Include Boost Directory
   ${Boost_INCLUDE_DIRS}

   # Include Eigen Directory
   $ENV{EIGEN_ROOT}
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
