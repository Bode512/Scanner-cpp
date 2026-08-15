#pragma once

#include "scan_types.hpp"
#include "scan_config.hpp"
#include <filesystem>
#include <system_error>

namespace scanner {

struct FileSizeInfo {
    uint64_t logical_size = 0;
    uint64_t allocated_size = 0;
    uint64_t hardlink_count = 0;
    FileIdentity identity;
    bool is_symlink = false;
    fs::path symlink_target;
};

class SizeCalculator {
public:
    SizeCalculator(const ScanConfig& config) : config_(config) {}

    // Obtiene información de tamaño e identidad para un archivo o symlink.
    // Devuelve error si falla.
    ScanResult<FileSizeInfo> get_file_info(const fs::path& path) const;

    // Obtiene información de tamaño para un directorio (solo identidad, no tamaño recursivo).
    ScanResult<FileIdentity> get_directory_identity(const fs::path& path) const;

private:
    const ScanConfig& config_;

    ScanResult<FileSizeInfo> get_file_info_platform(const fs::path& path) const;
};

} // namespace scanner