# Progreso y estado del escaneo

Este proyecto puede mostrar progreso durante el escaneo, pero hay que entender bien qué mide.

## 1. Callback de progreso

La API principal es:

```cpp
scanner.scan(sink, progress_callback);
```

El callback recibe un `ScanProgressSnapshot` con valores como:

- `files_visited`
- `directories_visited`
- `bytes_processed`
- `items_emitted`
- `errors`

Ejemplo:

```cpp
auto progress = [](const ScanProgressSnapshot& snap) {
    std::cerr << "\r[Progress] files=" << snap.files_visited
              << " dirs=" << snap.directories_visited
              << " bytes=" << snap.bytes_processed
              << " items=" << snap.items_emitted
              << " errors=" << snap.errors
              << std::flush;
};

scanner.scan(sink, progress);
```

## 2. Qué no hay todavía

Ahora mismo el proyecto no calcula un porcentaje total exacto, porque no conoce antes del recorrido el número total de archivos y directorios del árbol completo.

Por eso en salida verás cosas como:

```text
[Progress] files=12345 dirs=320 bytes=5210000 items=13000 errors=0
```

y no una barra del tipo:

```text
[====> 42%]
```

## 3. Cómo conseguir una barra de progreso real

Para tener un porcentaje real, necesitas una de estas dos opciones:

1. hacer un primer pase de conteo del árbol
2. usar un sistema de estadísticas acumuladas con un total previo

Eso se puede añadir más adelante, pero con la implementación actual el progreso es un contador en tiempo real, no una barra exacta.

## 4. Recomendación práctica

Para depuración o validación local:

```bash
./build/scanner_cli --workers 1 /tmp/scan.csv /home/bode
```

Esto te da una salida más clara y evita bloqueo por deadlock del pool en pruebas intensivas.

## 5. Qué usar para ver el estado real

- CLI: se ve en la salida estándar de error (`stderr`)
- C++: usa callback de progreso en `scanner.scan()`
- CSV: se genera cuando el scan termina y emite los items

## 6. Ejemplo completo

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
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
    std::cerr << "\nDone.\n";
    return 0;
}
```

Con esto ya lo ves funcionando en tiempo real, aunque no con porcentaje exacto.
