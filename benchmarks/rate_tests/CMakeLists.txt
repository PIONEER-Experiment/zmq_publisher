if(NOT DEFINED ENV{MIDASSYS})
  message(FATAL_ERROR "MIDASSYS is required to build the rate-test frontend")
endif()

add_executable(zmq_publisher_rate_frontend rate_frontend.cpp)
target_include_directories(zmq_publisher_rate_frontend PRIVATE
  $ENV{MIDASSYS}/include
  $ENV{MIDASSYS}/mxml
  $ENV{MIDASSYS}/midasio
)
target_link_libraries(zmq_publisher_rate_frontend PRIVATE
  Threads::Threads
  ${CMAKE_DL_LIBS}
  $ENV{MIDASSYS}/lib/libmfe.a
  $ENV{MIDASSYS}/lib/libmidas.a
  util
  rt
)
target_compile_options(zmq_publisher_rate_frontend PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers>
)

set(PIONEER_DETECTOR_SIMULATOR_SOURCES
  simulators/src/detector_frontend.cpp
  simulators/src/detector_payload_generator.cpp
  simulators/src/sampic_payload_generator.cpp
  simulators/src/hdsoc_payload_generator.cpp
  simulators/src/musip_payload_generator.cpp
  simulators/src/wfd5_payload_generator.cpp
)

foreach(detector IN ITEMS sampic musip hdsoc wfd5)
  add_executable(zmq_publisher_${detector}_simulator_frontend
    ${PIONEER_DETECTOR_SIMULATOR_SOURCES}
  )
  target_include_directories(zmq_publisher_${detector}_simulator_frontend PRIVATE
    simulators/include
    $ENV{MIDASSYS}/include
    $ENV{MIDASSYS}/mxml
    $ENV{MIDASSYS}/midasio
  )
  string(TOUPPER "${detector}" detector_upper)
  target_compile_definitions(zmq_publisher_${detector}_simulator_frontend PRIVATE
    PIONEER_SIMULATOR_${detector_upper}=1
  )
  target_link_libraries(zmq_publisher_${detector}_simulator_frontend PRIVATE
    Threads::Threads
    ${CMAKE_DL_LIBS}
    $ENV{MIDASSYS}/lib/libmfe.a
    $ENV{MIDASSYS}/lib/libmidas.a
    util
    rt
  )
  target_compile_options(zmq_publisher_${detector}_simulator_frontend PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic;-Wno-missing-field-initializers>
  )
endforeach()

add_executable(zmq_publisher_rate_subscriber
  rate_subscriber.cpp
  rate_subscriber_main.cpp
)
target_include_directories(zmq_publisher_rate_subscriber PRIVATE
  ${CPPZMQ_INCLUDE_DIR}
)
target_link_libraries(zmq_publisher_rate_subscriber PRIVATE
  nlohmann_json::nlohmann_json
  ${ZMQ_SYSTEM_LIBRARIES}
)
target_compile_options(zmq_publisher_rate_subscriber PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)
