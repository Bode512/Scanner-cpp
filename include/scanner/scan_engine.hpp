#pragma once

#include "scan_config.hpp"
#include "scan_types.hpp"
#include "result_sink.hpp"
#include "thread_pool.hpp"
#include "platform/platform_scanner.hpp"
#include <atomic>
#include <memory>
#include <stop_token>
#include <thread>
#include <unordered_set>
#include <mutex>

namespace scanner {

class ScanEngine {
public:
    ScanEngine(const ScanConfig& config);
    ~ScanEngine();

    ScanEngine(const ScanEngine&) = delete;
    ScanEngine& operator=(const ScanEngine&) = delete;

    // Ejecuta el escaneo. Bloquea hasta terminar o cancelar.
    // sink: recibe resultados en streaming.
    // progress: callback opcional para progreso periódico.
    // st: token de cancelación.
    void scan(ScanResultSink& sink,
              std::function<void(const ScanProgressSnapshot&)> progress,
              std::stop_token st);

private:
    const ScanConfig& config_;
    std::unique_ptr<PlatformScanner> platform_;
    std::unique_ptr<ThreadPool> pool_;

    // Contadores atómicos para progreso
    std::atomic<uint64_t> files_visited_{0};
    std::atomic<uint64_t> directories_visited_{0};
    std::atomic<uint64_t> bytes_processed_{0};
    std::atomic<uint64_t> items_emitted_{0};
    std::atomic<uint64_t> errors_{0};
    std::atomic<uint64_t> skipped_{0};
    std::atomic<fs::path*> current_path_{nullptr}; // puntero a path actual (protegido por atomicidad del puntero)

    // Mapa de identidades para hard links (device, inode) -> ya visto
    std::unordered_set<uint64_t> hardlink_seen_; // combinación en un uint64 mediante hash
    std::mutex hardlink_mutex_;

    // Cola de tareas de directorios pendientes (implementada en el pool)
    // Se usa una tarea por directorio raíz.

    void scan_root(const fs::path& root, ScanResultSink& sink, std::stop_token st);
    void process_directory(const fs::path& dir, ScanResultSink& sink, std::stop_token st);
    // Internal directory processor that returns total logical and allocated sizes
    std::pair<uint64_t, uint64_t> process_directory_internal(const fs::path& dir, ScanResultSink& sink, std::stop_token st);
    void process_file(const fs::path& file, ScanResultSink& sink, std::stop_token st);
    void process_symlink(const fs::path& link, ScanResultSink& sink, std::stop_token st);

    bool should_skip_path(const fs::path& path) const;
    bool check_hardlink(const FileIdentity& id);
    void emit_item(ScanResultSink& sink, ScanItem item);
    void report_error(ScanResultSink& sink, ScanError error);

    // Progreso
    void progress_loop(std::stop_token st,
                       std::function<void(const ScanProgressSnapshot&)> callback);

    // Eliminar recursión: usamos una pila de directorios a procesar.
    // En lugar de llamadas recursivas, cada directorio se encola como tarea.
};

} // namespace scanner