#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_set>
#include <thread>

namespace scanner {

struct ScanConfig {
    // Símbolos
    bool follow_symlinks = false;          // No seguir symlinks por defecto
    bool include_hidden = false;           // Omitir archivos ocultos (excepto en plataformas donde no hay concepto)
    bool include_system = false;           // Incluir archivos/directorios de sistema (p.ej. Windows "Windows", Linux "/proc")
    bool include_removable = true;         // Incluir unidades extraíbles
    bool include_network = false;          // Incluir filesystems de red (NFS, SMB, etc.)
    bool count_hardlinks_once = true;      // Evitar contar el mismo archivo múltiples veces (usa device+inode)

    // Concurrencia
    uint32_t worker_count = std::thread::hardware_concurrency();
    uint32_t max_queue_depth = 0;          // 0 = auto: worker_count * 4

    // Límites de profundidad (0 = sin límite)
    uint32_t max_depth = 0;

    // Filtros de paths (opcional)
    std::vector<std::string> excluded_paths; // Prefijos de rutas a excluir
    std::vector<std::string> included_paths; // Si no está vacío, solo se escanean estos

    // Tipos de filesystem a excluir (solo Linux/macOS; Windows maneja por unidad)
    std::unordered_set<std::string> excluded_fs_types = {
        "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "cgroup", "cgroup2",
        "pstore", "bpf", "autofs", "debugfs", "tracefs", "securityfs",
        "hugetlbfs", "mqueue", "configfs", "fusectl", "overlay",
        "squashfs", "ramfs", "binfmt_misc", "nsfs", "rpc_pipefs"
    };

    // Umbral de tamaño para emitir solo archivos grandes (0 = todos)
    uint64_t min_file_size = 0;

    // Frecuencia de actualización de progreso (ms)
    uint32_t progress_interval_ms = 500;

    // Configuración adicional
    bool emit_files = true;       // Emitir archivos
    bool emit_directories = true; // Emitir directorios (con tamaño agregado)
    bool emit_symlinks = false;   // Emitir symlinks como items
    bool emit_volumes = true;     // Emitir volúmenes raíz
};

} // namespace scanner