# Uso del scanner

Este documento explica cómo usar el escáner en modo CLI o desde código C++ y cómo activar la salida de progreso.

## 1. Escaneo desde la línea de comandos

El proyecto incluye un ejecutable llamado `scanner_cli`.

### Sintaxis

```bash
./build/scanner_cli <output.csv> <path1> [path2 ...] [opciones]
```

### Ejemplos

Escanear una sola carpeta:

```bash
./build/scanner_cli /tmp/scan.csv /home/bode
```

Escanear varias carpetas:

```bash
./build/scanner_cli /tmp/multi.csv /home/bode /home/usuario
```

Con número de workers:

```bash
./build/scanner_cli --workers 8 /tmp/scan.csv /home/bode
```

Sin archivos, solo directorios:

```bash
./build/scanner_cli --no-files /tmp/scan.csv /home/bode
```

Con límite de tamaño mínimo:

```bash
./build/scanner_cli --min-size 1048576 /tmp/scan.csv /home/bode
```

Con timeout:

```bash
./build/scanner_cli --timeout 30 /tmp/scan.csv /home/bode
```

## 2. Cómo funciona `included_paths`

Cuando pasas una ruta al CLI, el programa la mete dentro de `ScanConfig::included_paths`.
Esto hace que el scanner no recorra todo el sistema de archivos y se limite a esos directorios.

### En C++

```cpp
ScanConfig config;
config.included_paths.push_back("/home/bode");
```

Eso es lo importante para evitar escanear `/`, `/proc`, `/sys`, etc.

## 3. Escaneo desde código C++

### Ejemplo mínimo

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

    CsvSink sink("/tmp/result.csv");
    Scanner scanner(config);
    scanner.scan(sink);
    return 0;
}
```

### Ejemplo con progreso visible

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

    CsvSink sink("/tmp/result.csv");
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

## 4. Qué genera el CSV

La salida CSV contiene cabecera y filas con datos como:

```csv
path,type,logical_size,allocated_size,hardlink_count,is_symlink
"/home/bode/file.txt",0,123,4096,1,false
"/home/bode/docs",1,0,0,0,false
```

### Tipos

- `0` = File
- `1` = Directory
- `2` = Application
- `3` = Volume
- `4` = Symlink
- `5` = Other

## 5. Progreso: qué puedes ver y qué no

El escáner expone un callback de progreso con un `ScanProgressSnapshot` real. Eso permite ver:

- archivos visitados
- directorios visitados
- bytes procesados
- items emitidos
- errores

Lo que no tiene ahora mismo es un total global conocido antes de empezar. Por eso no puede calcular un porcentaje exacto ni tiempo restante fiable sin un dato previo de total de archivos/directorios.

Si quieres una barra porcentual real, tendrás que implementar una segunda etapa con:

- total estimado de archivos
- total estimado de directorios
- o recuento previo del árbol

## 6. Recomendaciones

- Para una prueba rápida: usa `--workers 1`
- Para carpetas grandes: usa rutas concretas y no `/`
- Para depurar: usa `--timeout` y `--workers 1`
- Para exportar: usa `scanner_cli` o `CsvSink`
- Para ver progreso real: usa el callback `scanner.scan(sink, progress)`

## 7. Ejecución real sobre tu home

```bash
./build/scanner_cli /home/bode/Documents/Dev/c/Scanner/scanner/home_bode_scan.csv /home/bode
```

Y luego puedes abrir el CSV generado con cualquier editor o con Excel/LibreOffice.
