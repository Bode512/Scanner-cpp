cmake_minimum_required(VERSION 3.15)
project(scanner)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
target_include_directories(scanner PUBLIC include include/scanner include/scanner/platform)

# Tests
add_executable(test_scanner tests/test_scan_engine.cpp)
target_link_libraries(test_scanner scanner)

# Benchmark
add_executable(benchmark benchmarks/benchmark_scan.cpp)
target_link_libraries(benchmark scanner)

add_executable(scanner_cli tools/scanner_cli.cpp)
target_link_libraries(scanner_cli scanner)