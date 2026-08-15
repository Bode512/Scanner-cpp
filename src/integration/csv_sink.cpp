#include "integration/csv_sink.hpp"

namespace scanner {

CsvSink::CsvSink(const std::filesystem::path& output_file)
    : file_(output_file, std::ios::out | std::ios::trunc) {
    if (!file_) {
        throw std::runtime_error("Cannot open CSV output file");
    }
}

CsvSink::~CsvSink() {
    if (file_.is_open()) file_.close();
}

void CsvSink::on_item(const ScanItem& item) {
    std::lock_guard lock(mutex_);
    if (!header_written_) {
        file_ << "path,type,logical_size,allocated_size,hardlink_count,is_symlink\n";
        header_written_ = true;
    }
    file_ << "\"" << item.path.string() << "\","
          << static_cast<int>(item.type) << ","
          << item.logical_size << ","
          << item.allocated_size << ","
          << item.hardlink_count << ","
          << (item.is_symlink ? "true" : "false") << "\n";
}

void CsvSink::on_error(const ScanError& error) {
    std::lock_guard lock(mutex_);
    // Opcional: escribir errores en un archivo separado o en stderr
    // Por ahora, los ignoramos o los escribimos en un log.
}

void CsvSink::on_scan_complete() {
    std::lock_guard lock(mutex_);
    file_.flush();
}

} // namespace scanner