cmake_minimum_required(VERSION 3.20)
project(swapcycle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-fcoroutines -Wall -Wextra)
endif()

find_package(Drogon CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)

# Everything except main.cc is shared with the tests. OBJECT (not STATIC) so
# the linker keeps every file: Drogon filters register themselves through
# static initialisers that nothing references directly.
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS src/*.cc)
list(REMOVE_ITEM CORE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc)

add_library(swapcycle_core OBJECT ${CORE_SOURCES})
target_include_directories(swapcycle_core PUBLIC src)
target_link_libraries(swapcycle_core PUBLIC Drogon::Drogon PkgConfig::SODIUM)

add_executable(swapcycle src/main.cc)
target_link_libraries(swapcycle PRIVATE swapcycle_core)

option(SWAPCYCLE_BUILD_TESTS "Build the GoogleTest suite" ON)
if(SWAPCYCLE_BUILD_TESTS)
  enable_testing()
  find_package(GTest REQUIRED)
  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS tests/*.cc)
  add_executable(swapcycle_tests ${TEST_SOURCES})
  target_include_directories(swapcycle_tests PRIVATE tests)
  target_link_libraries(swapcycle_tests PRIVATE swapcycle_core GTest::gtest_main)
  include(GoogleTest)
  gtest_discover_tests(swapcycle_tests)
endif()
