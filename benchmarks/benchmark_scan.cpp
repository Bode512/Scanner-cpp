#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>

using namespace scanner;
namespace fs = std::filesystem;

class StatsSink : public ScanResultSink {
public:
    explicit StatsSink(const fs::path& csv_path)
        : csv_sink_(csv_path) {}

    void on_item(const ScanItem& item) override {
        csv_sink_.on_item(item);

        if (item.type == ScanItemType::File) {
            file_count_.fetch_add(1, std::memory_order_relaxed);
            total_bytes_.fetch_add(item.logical_size, std::memory_order_relaxed);
        } else if (item.type == ScanItemType::Directory) {
            dir_count_.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void on_error(const ScanError& error) override {
        csv_sink_.on_error(error);
        errors_.fetch_add(1, std::memory_order_relaxed);
    }

    void on_scan_complete() override {
        csv_sink_.on_scan_complete();
    }

    size_t file_count() const { return file_count_.load(std::memory_order_relaxed); }
    size_t dir_count() const { return dir_count_.load(std::memory_order_relaxed); }
    size_t error_count() const { return errors_.load(std::memory_order_relaxed); }
    uint64_t total_bytes() const { return total_bytes_.load(std::memory_order_relaxed); }

private:
    CsvSink csv_sink_;
    std::atomic<size_t> file_count_{0};
    std::atomic<size_t> dir_count_{0};
    std::atomic<size_t> errors_{0};
    std::atomic<uint64_t> total_bytes_{0};
};

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Uso:\n"
                  << "  ./benchmark /home/bode\n"
                  << "  ./benchmark /home/bode /home/bode_scan.csv\n"
                  << "  ./benchmark /home/usuario /home/usuario_scan.csv\n";
        return 1;
    }

    fs::path root = argv[1];
    fs::path csv_path = (argc >= 3) ? fs::path(argv[2]) : root.parent_path() / (root.filename().string() + "_scan.csv");

    if (!fs::exists(root) || !fs::is_directory(root)) {
        std::cerr << "No existe o no es un directorio: " << root << "\n";
        return 2;
    }

    if (csv_path.empty() || csv_path == ".") {
        csv_path = fs::current_path() / "scan_report.csv";
    }

    fs::create_directories(csv_path.parent_path());

    ScanConfig config;
    config.included_paths.push_back(root.string());
    config.worker_count = std::max(1u, std::thread::hardware_concurrency() == 0 ? 1u : std::thread::hardware_concurrency());
    config.emit_files = true;
    config.emit_directories = true;
    config.emit_volumes = false;
    config.count_hardlinks_once = false;
    config.follow_symlinks = false;
    config.include_hidden = true;
    config.include_system = false;

    StatsSink sink(csv_path);
    Scanner scanner(config);

    auto start = std::chrono::steady_clock::now();
    scanner.scan(sink);
    auto end = std::chrono::steady_clock::now();

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    const double elapsed_s = elapsed_ms / 1000.0;
    const double mb_per_second = (elapsed_s > 0.0)
        ? (sink.total_bytes() / (1024.0 * 1024.0)) / elapsed_s
        : 0.0;

    std::cout << "ROOT: " << root << "\n";
    std::cout << "CSV: " << csv_path << "\n";
    std::cout << "FILES: " << sink.file_count() << "\n";
    std::cout << "DIRECTORIOS: " << sink.dir_count() << "\n";
    std::cout << "ERRORES: " << sink.error_count() << "\n";
    std::cout << "TIEMPO: " << elapsed_ms << " ms (" << elapsed_s << " s)\n";
    std::cout << "BYTES: " << sink.total_bytes() << "\n";
    std::cout << "MB/s: " << mb_per_second << "\n";
    return 0;
}