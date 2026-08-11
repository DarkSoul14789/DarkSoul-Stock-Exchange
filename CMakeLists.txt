cmake_minimum_required(VERSION 3.16)
project(orderbook_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra)
    add_compile_options("$<$<CONFIG:Release>:-O2>")
endif()

# --- Core library (matching engine logic) ---
add_library(orderbook_lib
    src/OrderBook.cpp
)
target_include_directories(orderbook_lib PUBLIC include)
if(NOT MSVC)
    find_package(Threads REQUIRED)
    target_link_libraries(orderbook_lib PUBLIC Threads::Threads)
endif()

# --- Benchmark executable ---
add_executable(benchmark src/benchmark.cpp)
target_link_libraries(benchmark PRIVATE orderbook_lib)

# --- Tests (Catch2, auto-fetched) ---
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit_tests tests/test_orderbook.cpp)
target_link_libraries(unit_tests PRIVATE orderbook_lib Catch2::Catch2WithMain)

enable_testing()
add_test(NAME unit_tests COMMAND unit_tests)