set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
cmake_minimum_required(VERSION 3.20)
project(bridge CXX C)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

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
  URL_HASH SHA256=c39a832624e041ea47ac198ef931d5d259d38bf56ecaa44239fad986cf4206f2
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 2. glog
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glog
  URL https://github.com/google/glog/archive/refs/tags/v0.7.1.tar.gz
  URL_HASH SHA256=00e4a87e87b7e7612f519a41e491f16623b12423620006f59f5688bfd8d13b08
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# 3. Catch2 (only needed for native test builds)
if(NOT EMSCRIPTEN)
  set(CATCH_BUILD_TESTING OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    Catch2
    URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.5.2.tar.gz
    URL_HASH SHA256=269543a49eb76f40b3f93ff231d4c24c27a7e16c90e47d2e45bcc564de470c6e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
endif()

# 4. DDS
FetchContent_Declare(
  dds
  URL https://github.com/dds-bridge/dds/archive/refs/tags/v2.9.0.tar.gz
  URL_HASH SHA256=9ef36d8c36bf697ba3b499fcb9dca51a4b423278ac72e947235ac86f0b5fc38a
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(EMSCRIPTEN)
  FetchContent_MakeAvailable(boost_headers glog)
else()
  FetchContent_MakeAvailable(boost_headers glog Catch2)
endif()

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
  target_include_directories(dds PUBLIC
    "${dds_SOURCE_DIR}/include"
    "${dds_SOURCE_DIR}/src"
  )
  if(EMSCRIPTEN)
    target_compile_options(dds PRIVATE -O3 -msimd128 -fexceptions)
    target_link_libraries(dds PUBLIC boost_headers)
  else()
    target_compile_definitions(dds PRIVATE DDS_THREADS_OPENMP)
    find_package(OpenMP REQUIRED)
    target_link_libraries(dds PUBLIC OpenMP::OpenMP_CXX boost_headers)
  endif()
endif()

set(BRIDGE_CORE_SOURCES
  string_util.cc
  card.cc
  hand.cc
  board_state.cc
  deal.cc
  hand_requirements.cc
  solver.cc
  conditional_line.cc
  line_finder.cc
  interface_dds.cc
  web/web_server.cc
  unknown_cards_invariant.cc
  cross_hand_summarizer.cc
  search_progress.cc
)

if(EMSCRIPTEN)
  add_library(bridge_lib STATIC ${BRIDGE_CORE_SOURCES})
  target_include_directories(bridge_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/web
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
  )
  target_compile_options(bridge_lib PUBLIC
    -O3
    -msimd128
    -fexceptions
    -Wno-unknown-pragmas
  )
  target_link_libraries(bridge_lib PUBLIC
    dds
    glog::glog
    boost_headers
  )

  add_executable(bridge_wasm web/wasm_main.cc)
  target_link_libraries(bridge_wasm PRIVATE bridge_lib)
  target_link_options(bridge_wasm PRIVATE
    "-O3"
    "-fexceptions"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sINITIAL_MEMORY=67108864"
    "-sMAXIMUM_MEMORY=2147483648"
    "-sSTACK_SIZE=8388608"
    "-sENVIRONMENT=worker,web,node"
    "-sEXPORTED_FUNCTIONS=['_wasm_solve_json','_wasm_get_solve_status','_wasm_stop_solve','_malloc','_free']"
    "-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
  )
else()
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
    ${BRIDGE_CORE_SOURCES}
    golden_serializer.cc
    web/web_assets.h
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
    string_util_test
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
endif()
