# 📖 Guía de Uso Completa — Scanner.cpp

Este documento describe todas las formas de utilizar **Scanner.cpp**, tanto a través de la herramienta CLI (`scanner_cli`) como mediante la API nativa C++20.

---

## 💻 1. Uso desde la Línea de Comandos (`scanner_cli`)

### Sintaxis General

```bash
./build/scanner_cli <output.csv> <ruta1> [ruta2 ...] [opciones]
```

### Tabla de Opciones CLI

| Opción | Descripción | Ejemplo |
| :--- | :--- | :--- |
| `--workers N` | Especifica el número de hilos de trabajo paralelos. | `--workers 8` |
| `--no-files` | Omite la emisión de archivos individuales. | `--no-files` |
| `--no-dirs` | Omite la emisión de directorios. | `--no-dirs` |
| `--include-hidden` | Incluye archivos y carpetas ocultas. | `--include-hidden` |
| `--follow-symlinks`| Sigue enlaces simbólicos (usar con precaución). | `--follow-symlinks` |
| `--min-size BYTES` | Filtra archivos menores a `BYTES`. | `--min-size 1048576` |
| `--timeout SEC` | Aborta automáticamente tras `SEC` segundos. | `--timeout 60` |
| `--help` | Muestra el mensaje de ayuda y opciones. | `--help` |

### Ejemplos de Comandos CLI

#### Escaneo Estándar de Home
```bash
./build/scanner_cli /tmp/home_report.csv /home/usuario
```

#### Escaneo Multi-ruta con 16 Hilos
```bash
./build/scanner_cli /tmp/system_report.csv /var /opt /srv --workers 16
```

#### Buscar solo Archivos Grandes (> 500 MB)
```bash
./build/scanner_cli /tmp/heavy_files.csv /home/usuario --min-size 524288000
```

---

## 🧩 2. Uso desde Código C++20

### Ejemplo A: Escaneo Básico con Exportación a CSV

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <iostream>

using namespace scanner;

int main() {
    ScanConfig config;
    config.included_paths.push_back("/home/usuario/Projects");
    config.worker_count = 8;
    config.emit_files = true;
    config.emit_directories = true;

    CsvSink sink("projects_scan.csv");
    Scanner scanner_engine(config);
    
    scanner_engine.scan(sink);
    std::cout << "Escaneo finalizado correctamente.\n";
    return 0;
}
```

### Ejemplo B: Captura de Progreso en Tiempo Real

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <iostream>

using namespace scanner;

int main() {
    ScanConfig config;
    config.included_paths.push_back("/home/usuario");
    config.progress_interval_ms = 250; // Callback cada 250ms

    CsvSink sink("home_scan.csv");
    Scanner scanner_engine(config);

    auto progress_handler = [](const ScanProgressSnapshot& snap) {
        std::cerr << "\r[PROGRESO] Archivos: " << snap.files_visited
                  << " | Carpetas: " << snap.directories_visited
                  << " | Bytes: " << snap.bytes_processed
                  << " | Errores: " << snap.errors << std::flush;
    };

    scanner_engine.scan(sink, progress_handler);
    std::cerr << "\nProceso completado.\n";
    return 0;
}
```

### Ejemplo C: Custom Thread-Safe Sink (Procesamiento Personalizado)

```cpp
#include "scanner/scanner.hpp"
#include <mutex>
#include <vector>
#include <iostream>

using namespace scanner;

class MemorySink : public ScanResultSink {
public:
    void on_item(const ScanItem& item) override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (item.type == ScanItemType::File) {
            total_bytes_ += item.logical_size;
            file_count_++;
        }
    }

    void on_error(const ScanError& error) override {
        std::lock_guard<std::mutex> lock(mutex_);
        errors_count_++;
    }

    uint64_t get_total_bytes() const { return total_bytes_; }
    uint64_t get_file_count() const { return file_count_; }

private:
    mutable std::mutex mutex_;
    uint64_t total_bytes_ = 0;
    uint64_t file_count_ = 0;
    uint64_t errors_count_ = 0;
};
```
