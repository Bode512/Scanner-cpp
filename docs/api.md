# 📚 Referencia de la API C++20 — Scanner.cpp

Documentación técnica detallada de las clases, estructuras, enumeraciones e interfaces públicas expuestas por el espacio de nombres `scanner`.

---

## 📋 Espacio de Nombres: `scanner`

Cabeceras de inclusión principales:

```cpp
#include "scanner/scanner.hpp"      // Motor principal
#include "scanner/scan_config.hpp"  // Opciones de configuración
#include "scanner/scan_types.hpp"   // Tipos de datos, items y snapshots
#include "scanner/result_sink.hpp"  // Interfaz de salida
#include "integration/csv_sink.hpp" // Implementación de salida a CSV
```

---

## 🛠️ Clase Principal: `scanner::Scanner`

La clase `Scanner` coordina el ciclo de vida del escaneo y la distribución de trabajo entre los hilos del pool.

### Constructor

```cpp
explicit Scanner(ScanConfig config);
```

Inicializa una instancia del motor utilizando las opciones especificadas en `config`.

### Métodos

```cpp
void scan(
    ScanResultSink& sink,
    std::function<void(const ScanProgressSnapshot&)> progress = nullptr,
    std::stop_token st = std::stop_token{}
);
```

Ejecuta el escaneo de directorios de manera síncrona en el hilo llamante mientras distribuye el trabajo interno entre el pool de hilos.

- **`sink`**: Referencia a un objeto que implementa `ScanResultSink` donde se enviarán los items descubiertos. Debe ser thread-safe.
- **`progress`**: Callback opcional invocado periódicamente con el estado actual del escaneo.
- **`st`**: Token de cancelación cooperativa (`std::stop_token`). Permite abortar el escaneo en cualquier momento.

---

## ⚙️ Estructura: `scanner::ScanConfig`

Define todos los parámetros configurables para controlar el comportamiento del escáner.

```cpp
struct ScanConfig {
    // Símbolos y Navegación
    bool follow_symlinks = false;     // Si es true, sigue los enlaces simbólicos.
    bool include_hidden = false;      // Si es true, procesa archivos y carpetas ocultos.
    bool include_system = false;      // Si es true, incluye directorios de sistema (/proc, Windows).
    bool include_removable = true;    // Si es true, incluye unidades extraíbles.
    bool include_network = false;     // Si es true, escanea sistemas de archivos en red (NFS/SMB).
    bool count_hardlinks_once = true; // Previene duplicar el tamaño físico asignado de hard links.

    // Concurrencia
    uint32_t worker_count = std::thread::hardware_concurrency(); // Hilos de trabajo.
    uint32_t max_queue_depth = 0;     // 0 = automático (worker_count * 4).

    // Filtros de Profundidad
    uint32_t max_depth = 0;           // Limite de profundidad (0 = sin límite).

    // Filtros de Ruta
    std::vector<std::string> excluded_paths; // Lista de prefijos de rutas a ignorar.
    std::vector<std::string> included_paths; // Rutas raíz específicas a escanear.

    // Exclusión de Sistemas de Archivos
    std::unordered_set<std::string> excluded_fs_types; // Tipos de FS a ignorar (proc, sysfs, etc).

    // Filtro por Tamaño
    uint64_t min_file_size = 0;        // Solo emite archivos de tamaño igual o superior (bytes).

    // Frecuencia de Progreso
    uint32_t progress_interval_ms = 500; // Intervalo de notificación del callback en milisegundos.

    // Configuración de Emisión
    bool emit_files = true;          // Emitir archivos individuales.
    bool emit_directories = true;    // Emitir directorios (con agregación de tamaño).
    bool emit_symlinks = false;      // Emitir symlinks como items individuales.
    bool emit_volumes = true;        // Emitir volúmenes raíz.
};
```

---

## 📄 Estructura: `scanner::ScanItem`

Representa la información obtenida para un archivo, directorio o entidad del sistema de archivos.

```cpp
struct ScanItem {
    fs::path path;               // Ruta absoluta normalizada.
    ScanItemType type;           // Tipo de elemento (File, Directory, Symlink, etc).
    uint64_t logical_size = 0;   // Tamaño lógico nominal en bytes.
    uint64_t allocated_size = 0; // Tamaño físico real ocupado en disco (bloques).
    FileIdentity identity;       // Identificador único (device + inode).
    uint64_t hardlink_count = 0; // Número total de enlaces duros.
    bool is_symlink = false;     // Indica si el elemento es un symlink.
    fs::path symlink_target;     // Destino del symlink (si aplica).
    std::optional<std::string> error_message; // Mensaje de error si la lectura falló.
};
```

---

## 🏷️ Enumeración: `scanner::ScanItemType`

```cpp
enum class ScanItemType {
    File,        // Archivo regular
    Directory,   // Directorio
    Application, // Paquete de aplicación (p.ej. .app en macOS)
    Volume,      // Punto de montaje o volumen raíz
    Symlink,     // Enlace simbólico
    Other        // Dispositivo de caracteres, socket, FIFO, etc.
};
```

---

## 📈 Estructura: `scanner::ScanProgressSnapshot`

Objeto entregado al callback de progreso con métricas en tiempo real:

```cpp
struct ScanProgressSnapshot {
    uint64_t files_visited = 0;       // Total de archivos examinados.
    uint64_t directories_visited = 0; // Total de directorios examinados.
    uint64_t bytes_processed = 0;     // Total de bytes leídos.
    uint64_t items_emitted = 0;       // Total de items enviados al Sink.
    uint64_t errors = 0;              // Total de errores de acceso/lectura encontrados.
    uint64_t skipped = 0;             // Total de elementos ignorados por filtros.
    fs::path current_path;            // Última ruta en procesamiento.
};
```

---

## 🔌 Interfaz: `scanner::ScanResultSink`

Interfaz abstracta que debe implementar cualquier receptor de resultados (exportadores, interfaces gráficas, bases de datos).

```cpp
class ScanResultSink {
public:
    virtual ~ScanResultSink() = default;

    // Invocado por cada ScanItem emitido (Debe ser THREAD-SAFE)
    virtual void on_item(const ScanItem& item) = 0;

    // Invocado cuando ocurre un error no fatal durante la lectura
    virtual void on_error(const ScanError& error) = 0;

    // Opcional: Invocado al finalizar el escaneo de una ruta raíz
    virtual void on_root_complete(const fs::path& root) { (void)root; }

    // Opcional: Invocado al finalizar todo el escaneo
    virtual void on_scan_complete() {}
};
```

---

## 📊 Clase: `scanner::CsvSink`

Implementación thread-safe de `ScanResultSink` para la escritura streaming en formato CSV.

### Constructor

```cpp
explicit CsvSink(const std::string& filename);
```

Crea o sobrescribe el archivo en `filename` y escribe la cabecera CSV estándar:
`path,type,logical_size,allocated_size,hardlink_count,is_symlink`
