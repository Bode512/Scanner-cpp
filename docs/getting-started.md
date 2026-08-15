# 🚀 Guía de Inicio Rápido — Scanner.cpp

Esta guía te ayudará a empezar a utilizar **Scanner.cpp** en menos de 5 minutos, ya sea como una herramienta de línea de comandos (CLI) o integrándolo como librería en tu proyecto C++20.

---

## 📌 Requisitos Previos

- **Compilador C++20**: GCC 10+, Clang 11+, o MSVC 2019+
- **CMake**: Versión 3.15 o superior
- **Sistema Operativo**: Linux, macOS o Windows

---

## 🛠️ 1. Compilación e Instalación Rápida

Clona e instala la herramienta en tu sistema en tres sencillos pasos:

```bash
# 1. Clonar el repositorio
git clone https://github.com/Bode512/Scanner-cpp.git
cd Scanner-cpp

# 2. Configurar y compilar binarios
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 3. Ejecutar pruebas de verificación
ctest --test-dir build --output-on-failure
```

Tras la compilación, se habrán generado tres ejecutable principales en el directorio `build/`:
- `build/scanner_cli`: Herramienta CLI interactiva.
- `build/test_scanner`: Ejecutable de pruebas unitarias.
- `build/benchmark`: Ejecutable de análisis de rendimiento.

---

## 💻 2. Uso mediante CLI (Herramienta de Línea de Comandos)

Para escanear un directorio y exportar los resultados a un archivo CSV:

```bash
# Escanear el directorio /var/log usando 8 hilos de procesamiento
./build/scanner_cli /tmp/logs_report.csv /var/log --workers 8
```

### Opciones CLI más utilizadas:

```bash
# Escanear omitiendo archivos de tamaño inferior a 1 MB (1048576 bytes)
./build/scanner_cli /tmp/large_files.csv /home/user --min-size 1048576

# Escanear únicamente directorios (omitir archivos individuales)
./build/scanner_cli /tmp/dirs_only.csv /home/user --no-files

# Abortar el escaneo si supera los 30 segundos de ejecución
./build/scanner_cli /tmp/scan.csv /var --timeout 30
```

---

## 🧩 3. Integración en Proyectos C++20

### Ejemplo Mínimo (`main.cpp`)

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <iostream>

using namespace scanner;

int main() {
    // 1. Configurar los parámetros de escaneo
    ScanConfig config;
    config.included_paths.push_back("/home/user/Documents");
    config.worker_count = std::thread::hardware_concurrency();
    config.emit_files = true;
    config.emit_directories = true;

    // 2. Definir el Sink de salida (Exportador a CSV)
    CsvSink sink("documents_scan.csv");

    // 3. Crear e invocar el motor de escaneo
    Scanner scanner_engine(config);
    scanner_engine.scan(sink);

    std::cout << "Escaneo completado exitosamente.\n";
    return 0;
}
```

### Compilar con CMake en tu proyecto

En tu archivo `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.15)
project(mi_proyecto CXX)

set(CMAKE_CXX_STANDARD 20)

# Incluir subdirectorio de scanner
add_subdirectory(path/to/scanner)

add_executable(mi_app main.cpp)
target_link_libraries(mi_app PRIVATE scanner::scanner)
```

---

## 📖 Próximos Pasos

- Lee la [Documentación de Arquitectura](architecture.md) para comprender el flujo interno multi-hilo.
- Consulta la [Referencia de la API](api.md) para explorar todas las clases e interfaces.
- Revisa las [Opciones de Configuración](configuration.md) para afinar el rendimiento del escáner.
