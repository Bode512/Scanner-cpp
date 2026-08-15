#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <cassert>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <iostream>

using namespace scanner;

void create_test_tree(const fs::path& root) {
    fs::create_directories(root / "dir1" / "subdir1");
    fs::create_directories(root / "dir2");
    std::ofstream(root / "file1.txt") << "Hello";
    std::ofstream(root / "dir1" / "file2.bin") << std::string(1000, 'A');
    std::ofstream(root / "dir1" / "subdir1" / "file3.txt") << "World";
    std::ofstream(root / "dir2" / "file4.txt") << "Test";
}

void test_basic_scan() {
    fs::path tmp = fs::temp_directory_path() / "scanner_test";
    fs::remove_all(tmp);
    create_test_tree(tmp);

    ScanConfig config;
    config.included_paths.push_back(tmp.string());
    config.worker_count = 2;
    config.follow_symlinks = false;
    config.include_hidden = false;
    config.include_system = false;
    config.emit_directories = true;
    config.emit_files = true;

    // Sink que cuenta items
    class TestSink : public ScanResultSink {
    public:
        std::vector<ScanItem> items;
        std::vector<ScanError> errors;
        void on_item(const ScanItem& item) override { items.push_back(item); }
        void on_error(const ScanError& error) override { errors.push_back(error); }
    } sink;

    Scanner scanner(config);
    std::stop_source stop_source;
    scanner.scan(sink, nullptr, stop_source.get_token());

    // Verificar que hay 5 archivos y 4 directorios (incluyendo root)
    size_t file_count = 0, dir_count = 0;
    for (const auto& item : sink.items) {
        if (item.type == ScanItemType::File) file_count++;
        if (item.type == ScanItemType::Directory) dir_count++;
    }
    assert(file_count == 4);
    assert(dir_count >= 4); // root + dir1 + subdir1 + dir2
    assert(sink.errors.empty());

    fs::remove_all(tmp);
}

void test_symlink_cycle() {
    // Similar, crear symlink cycle y verificar que no cuelga.
}

int main() {
    std::cout << "Start TEST. \n";
    test_basic_scan();
    test_symlink_cycle();
    std::cout << "All tests passed.\n";
    return 0;
}