# Scanner

Scanner es un escáner de archivos y directorios en C++20 que recorre rutas concretas, mide tamaños y exporta resultados a CSV.

## Qué hace

- Escanea rutas específicas de disco
- Recorre directorios y archivos de forma configurable
- Mide tamaño lógico y asignado
- Puede filtrar por rutas incluidas/excluidas
- Soporta workers de concurrencia
- Exporta resultados a CSV
- Tiene una CLI para usarlo sin escribir código
- Puede mostrar progreso en tiempo real mediante un callback

## Estructura del proyecto

- `include/` : cabeceras públicas
- `src/` : implementación del motor de escaneo
- `tools/` : CLI para uso directo
- `benchmarks/` : benchmarks de rendimiento
- `tests/` : pruebas unitarias
- `docs/` : documentación de uso y compilación

## 1. Compilar todo

Desde la raíz del proyecto:

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
cmake -S . -B build
cmake --build build
```

Esto genera:

- `build/test_scanner`
- `build/benchmark`
- `build/scanner_cli`

## 2. Ejecutar el CLI

Ejemplo básico:

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
./build/scanner_cli /tmp/scan.csv /home/bode
```

Esto hace un escaneo del directorio `/home/bode` y guarda el CSV en `/tmp/scan.csv`.

## 3. Ejecutar el benchmark

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
./build/benchmark /home/bode /home/bode/Documents/Dev/c/Scanner/scanner/home_bode_scan.csv
```

## 4. Ejecutar una prueba rápida

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
./build/test_scanner
```

## 5. Uso desde código C++

### Ejemplo mínimo sin progreso

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"

using namespace scanner;

int main() {
    ScanConfig config;
    config.included_paths.push_back("/home/bode");
    config.worker_count = 4;
    config.emit_files = true;
    config.emit_directories = true;

    CsvSink sink("result.csv");
    Scanner scanner(config);
    scanner.scan(sink);
    return 0;
}
```

Compilarlo:

```bash
cd /home/bode/Documents/Dev/c/Scanner
g++ -std=c++20 -pthread -I./scanner/include test.c++ ./scanner/build/libscanner.a -o test
./test
```

### Ejemplo con progreso en tiempo real

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <chrono>
#include <iostream>

using namespace scanner;

int main() {
    ScanConfig config;
    config.included_paths.push_back("/home/bode");
    config.worker_count = 4;
    config.emit_files = true;
    config.emit_directories = true;

    CsvSink sink("result.csv");
    Scanner scanner(config);

    auto progress = [](const ScanProgressSnapshot& snap) {
        std::cerr << "\r[Progress] files=" << snap.files_visited
                  << " dirs=" << snap.directories_visited
                  << " bytes=" << snap.bytes_processed
                  << " items=" << snap.items_emitted
                  << " errors=" << snap.errors
                  << std::flush;
    };

    scanner.scan(sink, progress);
    std::cerr << "\nScan complete.\n";
    return 0;
}
```

> El proyecto ofrece contadores vivos de progreso, pero no tiene un total global preconocido. Por eso no puede calcular un porcentaje preciso sin conocer el número total de archivos previo. Lo que sí da es estado en tiempo real: archivos vistos, directorios, bytes procesados y errores.

## 6. Documentación adicional

- [docs/usage.md](docs/usage.md)
- [docs/build.md](docs/build.md)
- [docs/progress.md](docs/progress.md)

## 7. Notas

- Si quieres escanear una carpeta concreta, usa `included_paths` o pásala como argumento al CLI.
- Si el árbol es muy grande, mejor usar `--workers 1` para depuración o pruebas.
- El CSV tiene cabecera con columnas como:
  - `path`
  - `type`
  - `logical_size`
  - `allocated_size`
  - `hardlink_count`
  - `is_symlink`
- En una depuración o un test local, es mejor usar `--workers 1` para ver claramente el progreso sin bloqueos por deadlock del pool.
