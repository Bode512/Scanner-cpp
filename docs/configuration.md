# ⚙️ Guía de Configuración — Scanner.cpp

Este documento detalla todas las opciones de configuración expuestas en `ScanConfig`, sus implicaciones en el rendimiento y los valores recomendados para distintos escenarios de producción.

---

## 📊 Matriz Completa de Opciones (`ScanConfig`)

| Parámetro | Tipo | Valor por Defecto | Descripción |
| :--- | :--- | :--- | :--- |
| `included_paths` | `std::vector<std::string>` | `[]` | Rutas absolutas concretas a escanear. Si se especifica, limita el escaneo a estas rutas. |
| `excluded_paths` | `std::vector<std::string>` | `[]` | Lista de prefijos de rutas que serán totalmente ignoradas durante la navegación. |
| `worker_count` | `uint32_t` | `std::thread::hardware_concurrency()` | Número de hilos de trabajo paralelos a instanciar en el `ThreadPool`. |
| `max_queue_depth` | `uint32_t` | `0` (auto = `worker_count * 4`) | Profundidad máxima de la cola de tareas antes de aplicar contrapresión (*backpressure*). |
| `max_depth` | `uint32_t` | `0` (sin límite) | Profundidad máxima de recursión en el árbol de directorios (0 = ilimitada). |
| `follow_symlinks` | `bool` | `false` | Indica si se deben seguir enlaces simbólicos. Usar con precaución. |
| `include_hidden` | `bool` | `false` | Indica si se deben incluir archivos y carpetas ocultas (dotfiles en Linux/macOS). |
| `include_system` | `bool` | `false` | Incluye directorios y archivos del sistema (`/proc`, `/sys`, `System32`). |
| `include_removable`| `bool` | `true` | Incluye unidades extraíbles o montajes de medios USB/SD. |
| `include_network`  | `bool` | `false` | Escanea puntos de montaje en red (NFS, SMB, CIFS, SSHFS). |
| `count_hardlinks_once` | `bool` | `true` | Deduplica inodos con múltiples enlaces duros para evitar inflar el tamaño físico asignado. |
| `min_file_size` | `uint64_t` | `0` | Filtro por tamaño mínimo (en bytes). Archivos menores a este tamaño no son emitidos. |
| `excluded_fs_types` | `std::unordered_set<std::string>` | Lista FS pseudo (proc, sysfs, etc) | Lista de tipos de sistemas de archivos virtuales/pseudo a ignorar en Linux/macOS. |
| `progress_interval_ms` | `uint32_t` | `500` | Intervalo en milisegundos para la emisión de instantáneas al callback de progreso. |
| `emit_files` | `bool` | `true` | Emitir items individuales de tipo `ScanItemType::File`. |
| `emit_directories` | `bool` | `true` | Emitir items de tipo `ScanItemType::Directory` con agregación de tamaño. |
| `emit_symlinks` | `bool` | `false` | Emitir items individuales para enlaces simbólicos. |
| `emit_volumes` | `bool` | `true` | Emitir los puntos de montaje raíz. |

---

## 🎯 Configuraciones Recomendadas por Caso de Uso

### 1. Auditoría de Espacio en Disco Rápida (Large Storage / NAS)

Para identificar qué directorios ocupan más espacio en volúmenes masivos:

```cpp
ScanConfig config;
config.included_paths.push_back("/mnt/storage");
config.worker_count = 16;         // Aprovechar concurrencia E/S NVMe/SSD
config.emit_files = false;        // Emitir solo directorios para reducir el volumen de salida
config.emit_directories = true;
config.min_file_size = 100 * 1024 * 1024; // Solo rastrear archivos > 100 MB
```

### 2. Generación de Inventario / Backup

Para capturar la estructura exacta incluyendo metadatos de enlaces duros:

```cpp
ScanConfig config;
config.included_paths.push_back("/home/user");
config.include_hidden = true;     // Incluir dotfiles (.config, .bashrc)
config.count_hardlinks_once = true;
config.emit_files = true;
config.emit_directories = true;
```

### 3. Entornos de Pruebas o Depuración (Single Thread)

Para evitar bloqueos o analizar el comportamiento paso a paso sin concurrencia:

```cpp
ScanConfig config;
config.included_paths.push_back("/tmp/test_dir");
config.worker_count = 1;          // Modo secuencial mono-hilo
```

---

## 🛑 Sistemas de Archivos Excluidos por Defecto (`excluded_fs_types`)

En sistemas POSIX (Linux), el escáner ignora por defecto sistemas de archivos pseudofísicos para evitar bucles infinitos, bloqueos de kernel o lectura de estructuras virtuales:

```json
[
  "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "cgroup", "cgroup2",
  "pstore", "bpf", "autofs", "debugfs", "tracefs", "securityfs",
  "hugetlbfs", "mqueue", "configfs", "fusectl", "overlay",
  "squashfs", "ramfs", "binfmt_misc", "nsfs", "rpc_pipefs"
]
```
