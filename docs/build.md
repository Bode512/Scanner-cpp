# 🔨 Guía de Compilación e Integración — Scanner.cpp

Este documento describe detalladamente cómo compilar el proyecto **Scanner.cpp**, configurar las opciones de CMake e integrarlo en proyectos externos.

---

## 📌 Requisitos del Sistema

| Herramienta | Versión Mínima | Notas |
| :--- | :---: | :--- |
| **Compilador C++** | C++20 | GCC 10+, Clang 11+, MSVC 2019+ |
| **CMake** | `>= 3.15` | Se recomienda 3.20+ |
| **Generador** | Any | Ninja, Make, Visual Studio |

---

## ⚙️ 1. Opciones de Configuración de CMake

Al invocar `cmake`, puedes ajustar las siguientes opciones mediante `-D<OPCIÓN>=<ON|OFF>`:

```bash
# Ejemplo: Compilar únicamente la librería y la CLI (sin tests ni benchmarks)
cmake -S . -B build -DBUILD_TESTING=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_CLI=ON
```

| Opción CMake | Valor Defecto | Descripción |
| :--- | :---: | :--- |
| `BUILD_TESTING` | `ON` | Compila la suite de pruebas unitarias (`test_scanner`). |
| `BUILD_BENCHMARKS` | `ON` | Compila el ejecutable de pruebas de rendimiento (`benchmark`). |
| `BUILD_CLI` | `ON` | Compila la herramienta ejecutable de línea de comandos (`scanner_cli`). |

---

## 🛠️ 2. Pasos de Compilación Estándar

```bash
# 1. Configurar
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 2. Compilar todos los objetivos en paralelo
cmake --build build --parallel

# 3. Ejecutar las pruebas unitarias con CTest
ctest --test-dir build --output-on-failure
```

---

## 📦 3. Integración en Proyectos Externos

### Método A: Usando CMake `FetchContent` (Recomendado)

En el `CMakeLists.txt` de tu aplicación:

```cmake
include(FetchContent)

FetchContent_Declare(
    scanner
    GIT_REPOSITORY https://github.com/Bode512/Scanner-cpp.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(scanner)

add_executable(mi_aplicacion main.cpp)
target_link_libraries(mi_aplicacion PRIVATE scanner::scanner)
```

### Método B: Usando `add_subdirectory`

Si tienes el repositorio de Scanner como submódulo Git o en un directorio local:

```cmake
add_subdirectory(vendor/scanner)

add_executable(mi_aplicacion main.cpp)
target_link_libraries(mi_aplicacion PRIVATE scanner::scanner)
```

### Método C: Enlace Manual mediante Compilador Directo

Si compilas un archivo individual como `test.cpp` contra la librería estática compilada:

```bash
g++ -std=c++20 -pthread -I./scanner/include test.cpp ./scanner/build/libscanner.a -o test_app
```
