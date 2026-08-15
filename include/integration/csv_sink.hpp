#pragma once

#include "scanner/result_sink.hpp"
#include <fstream>
#include <mutex>

namespace scanner {

class CsvSink : public ScanResultSink {
public:
    explicit CsvSink(const std::filesystem::path& output_file);
    ~CsvSink() override;

    void on_item(const ScanItem& item) override;
    void on_error(const ScanError& error) override;
    void on_scan_complete() override;

private:
    std::ofstream file_;
    std::mutex mutex_;
    bool header_written_ = false;
};

} // namespace scanner