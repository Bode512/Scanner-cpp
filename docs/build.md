# Compilar y ejecutar el proyecto

Este documento explica cómo montar el proyecto desde cero en Linux y cómo compilar un ejemplo externo con el scanner.

## 1. Requisitos

- CMake >= 3.15
- compilador C++20
- make o ninja
- sistema Linux

## 2. Configurar proyecto

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
cmake -S . -B build
```

## 3. Compilar todo

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
cmake --build build
```

Esto genera los binarios principales:

- `build/test_scanner`
- `build/benchmark`
- `build/scanner_cli`

## 4. Ejecutar los binarios

### Test

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
./build/test_scanner
```

### Benchmark

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
./build/benchmark /home/bode /home/bode/Documents/Dev/c/Scanner/scanner/home_bode_scan.csv
```

### CLI

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
./build/scanner_cli /tmp/scan.csv /home/bode
```

## 5. Compilar un target concreto

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
cmake --build build --target benchmark
cmake --build build --target test_scanner
cmake --build build --target scanner_cli
```

## 6. Compilar un fichero externo `test.c++`

Cuando compiles un archivo fuera del proyecto, debes enlazar la librería del scanner y añadir el include path.

### Ejemplo

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

> Si no enlazas `./scanner/build/libscanner.a`, el compilador te dará errores de `undefined reference`.

## 7. CMake del proyecto

El proyecto usa un `CMakeLists.txt` con:

- librería `scanner`
- ejecutable `test_scanner`
- ejecutable `benchmark`
- ejecutable `scanner_cli`

## 8. Resolver problemas comunes

### Error de rutas vacías

Si buscas escanear una carpeta concreta, asegúrate de pasarla como argumento o rellenar `included_paths`.

### El proceso se queda colgado

- usa `--workers 1` para depurar
- asegura que `included_paths` no está vacío
- evita escanear `/` sin filtrar

### CSV vacío

- confirma que la ruta objetivo existe
- confirma que `scanner.scan(...)` está ejecutándose
- revisa que `CsvSink` tiene la cabecera y que los items se están emitiendo

### No aparece progreso

- usa `scanner.scan(sink, progress)` con un callback
- si no pasas callback, no habrá salida en tiempo real
- el proyecto muestra contadores, no un porcentaje global exacto

## 9. Flujo recomendado

```bash
cd /home/bode/Documents/Dev/c/Scanner/scanner
cmake -S . -B build
cmake --build build
./build/scanner_cli /tmp/scan.csv /home/bode
```

Eso será suficiente para compilar y ejecutar el escáner real sobre una carpeta concreta.
