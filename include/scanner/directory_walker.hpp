#pragma once

#include "scan_types.hpp"
#include "scan_config.hpp"
#include <filesystem>
#include <functional>
#include <stop_token>

namespace scanner {

// Callback para cada entrada encontrada.
// Devuelve void.
using EntryCallback = std::function<void(const fs::directory_entry& entry)>;

// Walker que recorre un directorio sin recursión (usa pila explícita).
// No calcula tamaños, solo descubre entradas.
class DirectoryWalker {
public:
    DirectoryWalker(const ScanConfig& config) : config_(config) {}

    // Recorre el directorio raíz, llamando al callback por cada entrada (archivos, subdirectorios, symlinks).
    // Si follow_symlinks es false, los symlinks se tratan como entradas de tipo symlink.
    // Si follow_symlinks es true, los symlinks a directorios se recorren (con protección de ciclos mediante un conjunto de canonical paths).
    // El stop_token permite cancelación.
    void walk(const fs::path& root, const EntryCallback& callback, std::stop_token st);

private:
    const ScanConfig& config_;

    void walk_internal(const fs::path& root, const EntryCallback& callback, std::stop_token st);
};

} // namespace scanner