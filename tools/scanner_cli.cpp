#include "scanner/scanner.hpp"
#include "scanner/scan_config.hpp"
#include "scanner/result_sink.hpp"
#include "scanner/scan_types.hpp"
#include "integration/csv_sink.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

namespace {

std::atomic<bool> g_stop{false};

void signal_handler(int) {
    g_stop = true;
}

void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog << " <output.csv> <path1> [path2 ...]\n";
    std::cerr << "Scans the given paths and writes results to CSV.\n";
    std::cerr << "Options:\n";
    std::cerr << "  --workers N       Number of worker threads (default: hardware_concurrency)\n";
    std::cerr << "  --no-files        Do not emit individual files\n";
    std::cerr << "  --no-dirs         Do not emit directories\n";
    std::cerr << "  --include-hidden  Include hidden files/directories\n";
    std::cerr << "  --follow-symlinks Follow symlinks (use with caution)\n";
    std::cerr << "  --min-size BYTES  Only emit files >= BYTES\n";
    std::cerr << "  --timeout SEC     Abort after SEC seconds\n";
    std::cerr << "  --help            Show this help\n";
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace scanner;

    // Parse arguments
    if (argc < 3) {
        print_usage(argv[0]);
        return 2;
    }

    std::string output_csv;
    std::vector<fs::path> roots;
    ScanConfig config;
    uint32_t timeout_seconds = 0;
    bool show_help = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            show_help = true;
        } else if (arg == "--workers") {
            if (i + 1 < argc) {
                config.worker_count = static_cast<uint32_t>(std::stoul(argv[++i]));
            }
        } else if (arg == "--no-files") {
            config.emit_files = false;
        } else if (arg == "--no-dirs") {
            config.emit_directories = false;
        } else if (arg == "--include-hidden") {
            config.include_hidden = true;
        } else if (arg == "--follow-symlinks") {
            config.follow_symlinks = true;
        } else if (arg == "--min-size") {
            if (i + 1 < argc) {
                config.min_file_size = std::stoull(argv[++i]);
            }
        } else if (arg == "--timeout") {
            if (i + 1 < argc) {
                timeout_seconds = static_cast<uint32_t>(std::stoul(argv[++i]));
            }
        } else {
            if (output_csv.empty()) {
                output_csv = arg;
            } else {
                roots.emplace_back(arg);
            }
        }
    }

    if (show_help || output_csv.empty() || roots.empty()) {
        print_usage(argv[0]);
        return 2;
    }

    config.included_paths.clear();
    for (const auto& root : roots) {
        config.included_paths.push_back(root.string());
    }

    // Configure sink
    scanner::CsvSink sink(output_csv);

    // CsvSink does not define a conversion to bool; we only need to verify that the file opened.
    // If construction throws, the exception is handled below.

    // Setup cancellation via SIGINT
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::stop_source stop_source;
    std::stop_token stop_token = stop_source.get_token();

    // If timeout requested, spawn a thread to request stop after timeout
    std::jthread timeout_thread;
    if (timeout_seconds > 0) {
        timeout_thread = std::jthread([&stop_source, timeout_seconds]() {
            std::this_thread::sleep_for(std::chrono::seconds(timeout_seconds));
            stop_source.request_stop();
        });
    }

    // Progress callback (optional): print periodic progress to stderr
    
    auto progress_cb = [](const ScanProgressSnapshot& snap) {
        std::cerr << "\r[Progress] files=" << snap.files_visited
                  << " dirs=" << snap.directories_visited
                  << " bytes=" << snap.bytes_processed
                  << " items=" << snap.items_emitted
                  << " errors=" << snap.errors << std::flush;
    };

    // Run scan
    try {
        Scanner scanner(config);
        scanner.scan(sink, progress_cb, stop_token);
    } catch (const std::exception& e) {
        std::cerr << "\nException: " << e.what() << "\n";
        return 1;
    }
    std::cerr << "\nScan completed.\n";
    return 0;
}