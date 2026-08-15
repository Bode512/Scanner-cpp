cmake_minimum_required(VERSION 3.15)
project(scanner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(BUILD_TESTING "Build the test suite" ON)
option(BUILD_BENCHMARKS "Build the benchmark suite" ON)
option(BUILD_CLI "Build the scanner CLI application" ON)

# Library definition
add_library(scanner
    src/scan_engine.cpp
    src/directory_walker.cpp
    src/size_calculator.cpp
    src/thread_pool.cpp
    src/scanner.cpp
    src/integration/csv_sink.cpp
    src/platform/windows_scanner.cpp
    src/platform/linux_scanner.cpp
    src/platform/macos_scanner.cpp
    src/platform/platform_scanner_factory.cpp
)

add_library(scanner::scanner ALIAS scanner)

target_include_directories(scanner PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/scanner>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/scanner/platform>
    $<INSTALL_INTERFACE:include>
)

# Platform threading
find_package(Threads REQUIRED)
target_link_libraries(scanner PUBLIC Threads::Threads)

# CLI Target
if(BUILD_CLI)
    add_executable(scanner_cli tools/scanner_cli.cpp)
    target_link_libraries(scanner_cli PRIVATE scanner)
endif()

# Test Suite
if(BUILD_TESTING)
    enable_testing()
    add_executable(test_scanner tests/test_scan_engine.cpp)
    target_link_libraries(test_scanner PRIVATE scanner)
    add_test(NAME scan_engine_test COMMAND test_scanner)
endif()

# Benchmark Suite
if(BUILD_BENCHMARKS)
    add_executable(benchmark benchmarks/benchmark_scan.cpp)
    target_link_libraries(benchmark PRIVATE scanner)
endif()