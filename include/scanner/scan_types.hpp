#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>
#include <optional>
#include <variant>

namespace scanner {

namespace fs = std::filesystem;

enum class ScanItemType {
    File,
    Directory,
    Application,   // Solo cuando realmente se identifica como aplicación (p.ej. .app en macOS, con metadata)
    Volume,        // Unidad/montaje raíz
    Symlink,       // Enlace simbólico (si se decide emitir)
    Other
};

struct FileIdentity {
    uint64_t device = 0;
    uint64_t inode = 0;   // En Windows: FileIndex
    bool valid = false;
};

struct ScanItem {
    fs::path path;               // Ruta absoluta
    ScanItemType type = ScanItemType::Other;
    uint64_t logical_size = 0;   // Tamaño lógico en bytes
    uint64_t allocated_size = 0; // Tamaño asignado en disco (si está disponible)
    FileIdentity identity;       // Identidad para hard links (device+inode)
    uint64_t hardlink_count = 0; // Número de enlaces duros (si se conoce)
    bool is_symlink = false;
    fs::path symlink_target;     // Si es symlink, el destino
    std::optional<std::string> error_message; // Si hubo un error al medir
};

enum class ScanErrorCode {
    PermissionDenied,
    FileNotFound,
    DirectoryNotFound,
    BrokenSymlink,
    IoError,
    FilesystemUnavailable,
    InvalidPath,
    UnsupportedFilesystem,
    Cancelled,
    Unknown
};

struct ScanError {
    ScanErrorCode code = ScanErrorCode::Unknown;
    fs::path path;
    std::string message;
    std::error_code system_error; // Error del sistema subyacente

    ScanError() = default;
    ScanError(ScanErrorCode c, fs::path p, std::string m, std::error_code ec = {})
        : code(c), path(std::move(p)), message(std::move(m)), system_error(std::move(ec)) {}
};

struct ScanProgressSnapshot {
    uint64_t files_visited = 0;
    uint64_t directories_visited = 0;
    uint64_t bytes_processed = 0;
    uint64_t items_emitted = 0;
    uint64_t errors = 0;
    uint64_t skipped = 0;
    fs::path current_path;        // Último directorio en proceso (para depuración)
};

// Para compatibilidad con C++20/23: std::expected no está en C++20.
// Definimos un resultado simple basado en std::variant.
template<typename T>
class ScanResult {
public:
    ScanResult(T value) : data_(std::move(value)) {}
    ScanResult(ScanError error) : data_(std::move(error)) {}

    bool has_value() const { return std::holds_alternative<T>(data_); }
    T& value() & { return std::get<T>(data_); }
    const T& value() const & { return std::get<T>(data_); }
    T&& value() && { return std::get<T>(std::move(data_)); }
    ScanError& error() & { return std::get<ScanError>(data_); }
    const ScanError& error() const & { return std::get<ScanError>(data_); }

private:
    std::variant<T, ScanError> data_;
};

} // namespace scanner