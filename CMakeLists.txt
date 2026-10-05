set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.20)
project(bridge CXX C)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
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
  URL https://github.com/google/glog/archive/refs/tags/v0.7.1.tar.gz
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

find_package(Python3 REQUIRED COMPONENTS Interpreter)

FetchContent_GetProperties(dds)
if(NOT dds_POPULATED)
  FetchContent_Populate(dds)
  execute_process(
    COMMAND ${Python3_EXECUTABLE}
      "${CMAKE_CURRENT_SOURCE_DIR}/third_party/patch_dds.py"
      "${dds_SOURCE_DIR}"
    RESULT_VARIABLE patch_dds_result
  )
  if(NOT patch_dds_result EQUAL 0)
    message(FATAL_ERROR "Failed to patch DDS sources")
  endif()
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

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/web/web_assets.h
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/web/generate_web_assets.py
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/web/static/index.html
    ${CMAKE_CURRENT_SOURCE_DIR}/web/static/styles.css
    ${CMAKE_CURRENT_SOURCE_DIR}/web/static/app.js
    ${CMAKE_CURRENT_SOURCE_DIR}/web/presets.json
  COMMENT "Generating web/web_assets.h from static web assets"
)

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
  web/web_server.cc
  web/web_assets.h
  unknown_cards_invariant.cc
  cross_hand_summarizer.cc
  search_progress.cc
)

target_include_directories(bridge_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/web
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
add_executable(bridge_server web/server_main.cc)
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
  search_progress_test
)

foreach(test_name IN LISTS TEST_NAMES)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/web/${test_name}.cc")
    set(TEST_SRC "web/${test_name}.cc")
  else()
    set(TEST_SRC "${test_name}.cc")
  endif()
  add_executable(${test_name} ${TEST_SRC})
  target_link_libraries(${test_name} PRIVATE
    bridge_lib
    Catch2::Catch2WithMain
  )
  add_test(NAME ${test_name} COMMAND ${test_name} WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endforeach()
