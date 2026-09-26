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

add_executable(zmq_publisher_rate_subscriber
  rate_subscriber.cpp
  rate_subscriber_main.cpp
)
target_include_directories(zmq_publisher_rate_subscriber PRIVATE
  ${CPPZMQ_INCLUDE_DIR}
)
target_link_libraries(zmq_publisher_rate_subscriber PRIVATE
  nlohmann_json::nlohmann_json
  ${ZMQ_LIBRARY}
)
target_compile_options(zmq_publisher_rate_subscriber PRIVATE
  $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)
