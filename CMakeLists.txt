cmake_minimum_required(VERSION 3.20)
project(bridge CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# 1. Boost Headers
FetchContent_Declare(
  boost_headers
  URL https://github.com/boostorg/headers/archive/refs/tags/boost-1.83.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 2. glog
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glog
  URL https://github.com/google/glog/archive/refs/tags/v0.6.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 3. Catch2
set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  Catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 4. DDS
FetchContent_Declare(
  dds
  URL https://github.com/dds-bridge/dds/archive/refs/tags/v2.9.0.tar.gz
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(boost_headers glog Catch2)

if(NOT TARGET Boost::thread)
  add_library(Boost::thread ALIAS boost_headers)
endif()
if(NOT TARGET Boost::system)
  add_library(Boost::system ALIAS boost_headers)
endif()
if(NOT TARGET Boost::headers)
  add_library(Boost::headers ALIAS boost_headers)
endif()

FetchContent_GetProperties(dds)
if(NOT dds_POPULATED)
  FetchContent_Populate(dds)
  file(GLOB DDS_SOURCES "${dds_SOURCE_DIR}/src/*.cpp")
  add_library(dds STATIC ${DDS_SOURCES})
  target_compile_definitions(dds PRIVATE DDS_THREADS_OPENMP)
  target_include_directories(dds PUBLIC
    "${dds_SOURCE_DIR}/include"
    "${dds_SOURCE_DIR}/src"
  )
  find_package(OpenMP REQUIRED)
  target_link_libraries(dds PUBLIC OpenMP::OpenMP_CXX boost_headers)
endif()

# Core bridge solver library
add_library(bridge_lib STATIC
  card.cc
  hand.cc
  board_state.cc
  deal.cc
  hand_requirements.cc
  solver.cc
  conditional_line.cc
  line_finder.cc
  interface_dds.cc
  golden_serializer.cc
  web_server.cc
  cross_hand_summarizer.cc
)

target_include_directories(bridge_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

target_link_libraries(bridge_lib PUBLIC
  dds
  glog::glog
  boost_headers
  OpenMP::OpenMP_CXX
  Catch2::Catch2
)

if(WIN32)
  target_link_libraries(bridge_lib PUBLIC ws2_32 crypt32)
endif()

# Executable: bridge_server
add_executable(bridge_server server_main.cc)
target_link_libraries(bridge_server PRIVATE bridge_lib)

# Executable: line_finder_perf
add_executable(line_finder_perf perf/line_finder_perf.cc)
target_link_libraries(line_finder_perf PRIVATE bridge_lib)

enable_testing()

set(TEST_NAMES
  basic_test
  board_state_test
  conditional_line_test
  dds_test
  deal_test
  hand_requirements_test
  interface_dds_test
  line_finder_test
  server_test
  cross_hand_summarizer_test
)

foreach(test_name IN LISTS TEST_NAMES)
  add_executable(${test_name} ${test_name}.cc)
  target_link_libraries(${test_name} PRIVATE
    bridge_lib
    Catch2::Catch2WithMain
  )
  add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
