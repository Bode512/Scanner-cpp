# Guía de Contribución a Scanner.cpp

¡Gracias por tu interés en contribuir a **Scanner.cpp**! Apreciamos las contribuciónes de todo tipo: corrección de errores, mejoras de rendimiento, optimizaciones de la arquitectura multi-hilo, adición de nuevas plataformas o documentación.

Este documento proporciona las directrices y estándares para colaborar de manera efectiva en el proyecto.

---

## 📋 Tabla de Contenidos

- [Código de Conducta](#-código-de-conducta)
- [¿Cómo puedo contribuir?](#-cómo-puedo-contribuir)
  - [Reportar un error (Bug Report)](#reportar-un-error-bug-report)
  - [Proponer una funcionalidad (Feature Request)](#proponer-una-funcionalidad-feature-request)
  - [Enviar un Pull Request](#enviar-un-pull-request)
- [Configuración del Entorno Local](#-configuración-del-entorno-local)
- [Estándares de Código y Normas Técnicas](#-estándares-de-código-y-normas-técnicas)
- [Flujo de Commits (Conventional Commits)](#-flujo-de-commits-conventional-commits)
- [Ejecución de Pruebas y Validación](#-ejecución-de-pruebas-y-validación)

---

## 📜 Código de Conducta

Este proyecto adopta el **Contributor Covenant (v2.1)**. Al participar, te comprometes a mantener un entorno respetuoso, colaborativo e inclusivo. Por favor, consulta [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) para más detalle.

---

## 💡 ¿Cómo puedo contribuir?

### Reportar un error (Bug Report)

Antes de crear un nuevo reporte de error:
1. Revisa los [Issues abiertos](https://github.com/Bode512/Scanner-cpp/issues) para asegurarte de que el problema no ha sido reportado previamente.
2. Si no existe un issue previo, crea uno utilizando la plantilla de **Bug Report** proporcionando:
   - Sistema operativo, versión de compilador (GCC, Clang, MSVC) y versión de CMake.
   - Pasos detallados para reproducir el problema.
   - Comportamiento esperado frente al comportamiento obtenido.
   - Traza de error, logs o salida por consola relevante.

### Proponer una funcionalidad (Feature Request)

Si tienes una idea para mejorar el motor de escaneo, optimizar el pool de hilos o agregar soporte para nuevos sinks:
1. Abre un issue de **Feature Request**.
2. Explica el problema real o el caso de uso que la nueva característica resuelve.
3. Describe la arquitectura o diseño propuesto antes de escribir grandes volúmenes de código.

### Enviar un Pull Request

1. Haz un **Fork** del repositorio.
2. Crea una rama descriptiva para tu cambio:
   ```bash
   git checkout -b feat/lock-free-work-queue
   ```
3. Implementa los cambios asegurando la compatibilidad con el estándar C++20.
4. Escribe o actualiza las pruebas unitarias en `tests/`.
5. Ejecuta la suite de pruebas localmente (`ctest`).
6. Envía tu Pull Request a la rama `main` utilizando la plantilla de PR.

---

## 🛠️ Configuración del Entorno Local

### Requisitos

- **Compilador C++20**: GCC 10+, Clang 11+, o MSVC 2019+
- **CMake**: >= 3.15
- **Sistema Operativo**: Linux, macOS o Windows
- **Generador de Build**: `make`, `ninja`, o Visual Studio IDE

### Pasos de compilación

```bash
# 1. Clonar el repositorio
git clone https://github.com/Bode512/Scanner-cpp.git
cd Scanner-cpp

# 2. Configurar el proyecto con CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# 3. Compilar la librería, la CLI, los tests y benchmarks
cmake --build build --parallel

# 4. Ejecutar la suite de pruebas
ctest --test-dir build --output-on-failure
```

---

## 📐 Estándares de Código y Normas Técnicas

Para mantener un código limpio, idiomático y mantenible:

- **Estándar C++**: Usar C++20 de forma idiomática (`std::jthread`, `std::stop_token`, `std::filesystem`, `std::variant`, `std::optional`, `concepts`).
- **Gestión de Memoria y Recursos**: Principio RAII estricto. Evitar uso explícito de `new` / `delete`. Usar punteros inteligentes (`std::unique_ptr`, `std::shared_ptr`) cuando sea necesario.
- **Concurrencia**:
  - Garantizar la seguridad entre hilos (*thread-safety*) en cualquier implementación de `ScanResultSink`.
  - Evitar bloqueos (*deadlocks*) mediante orden consistente de mutexes o estructuras libres de bloqueos (*lock-free*).
  - Respetar la cancelación cooperativa mediante `std::stop_token`.
- **Nombrado**:
  - `snake_case` para funciones, métodos, miembros de estructuras y variables.
  - `PascalCase` para clases, tipos y enumeraciones (`ScanEngine`, `ScanItemType`).
  - Encabezados `.hpp` para cabeceras y `.cpp` para implementaciones.

---

## 🔀 Flujo de Commits (Conventional Commits)

Utilizamos la convención [Conventional Commits](https://www.conventionalcommits.org/es/v1.0.0/):

| Prefijo | Uso | Ejemplo |
| :--- | :--- | :--- |
| `feat` | Nueva funcionalidad | `feat(engine): add maximum recursion depth filter` |
| `fix` | Corrección de un error | `fix(thread_pool): prevent deadlock on task queue teardown` |
| `docs` | Cambios en la documentación | `docs(readme): update quick start guide and API references` |
| `perf` | Optimización de rendimiento | `perf(linux): accelerate inode stat lookup via statx` |
| `refactor` | Refactorización de código | `refactor(sink): simplify streaming CSV sink interface` |
| `test` | Adición o corrección de pruebas | `test(engine): add test case for symlink loop traversal` |
| `ci` | Cambios en integración continua | `ci(github): add multi-platform build workflow` |

---

## 🧪 Ejecución de Pruebas y Validación

Cualquier cambio debe pasar la suite de pruebas sin advertencias ni errores:

```bash
# Compilar y ejecutar pruebas
cmake --build build --target test_scanner
./build/test_scanner

# O usar CTest
ctest --test-dir build --output-on-failure
```

---

¡Gracias por contribuir a la comunidad Open Source! 🚀
