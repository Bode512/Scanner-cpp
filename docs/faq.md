# 💬 Preguntas Frecuentes (FAQ) — Scanner.cpp

---

### ¿Qué problema resuelve Scanner.cpp?

**Scanner.cpp** resuelve la necesidad de analizar y escanear rápidamente grandes volúmenes de datos y millones de archivos en discos locales o de red, calculando el tamaño lógico y el espacio asignado en disco con alta concurrencia C++20 y sin agotar la memoria RAM del sistema.

---

### ¿En qué se diferencia de comandos como `du`, `find` o `tree`?

- **Rendimiento Multi-hilo**: A diferencia de `du` o `find` tradicionales que operan en un solo hilo secuencial, **Scanner.cpp** utiliza un pool de hilos dinámico (`std::jthread`) que paraleliza las operaciones de E/S de discos NVMe/RAID modernos.
- **Exportación Streaming a CSV**: Escribe los resultados directamente en streaming a un Sink (como CSV) en lugar de acumular todos los objetos en RAM.
- **Deduplicación de Hard Links**: Realiza un seguimiento preciso de las identidades `(device, inode)` para no contabilizar múltiples veces el espacio asignado por hard links.
- **Diferenciación Lógico vs Físico**: Mide tanto los bytes nominales del archivo como los bloques reales asignados en el sistema de archivos (útil para sparse files y compresión).

---

### ¿Qué plataformas y compiladores son compatibles?

- **Sistemas Operativos**: Linux (kernels 4.x+), macOS (10.15+ / Apple Silicon & Intel), Windows 10 / 11 / Server.
- **Compiladores**: GCC 10+, Clang 11+, MSVC 2019+.
- **Estándar C++**: C++20 obligatorio.

---

### ¿Cómo puedo usar Scanner.cpp como librería en mi proyecto CMake?

Puedes integrarlo fácilmente agregándolo como subdirectorio o utilizando `FetchContent` de CMake:

```cmake
include(FetchContent)
FetchContent_Declare(
    scanner
    GIT_REPOSITORY https://github.com/Bode512/Scanner-cpp.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(scanner)

target_link_libraries(tu_proyecto PRIVATE scanner::scanner)
```

---

### ¿Cómo cancelo un escaneo en curso?

El motor acepta un `std::stop_token` de C++20. Al invocar `stop_source.request_stop()`, todos los hilos de trabajo detienen cooperativamente el procesamiento de tareas de forma inmediata y limpia.

---

### ¿Es thread-safe la interfaz `ScanResultSink`?

Sí. El método `on_item(const ScanItem& item)` de tu implementación personalizada de `ScanResultSink` puede ser invocado concurrentemente por múltiples hilos de trabajo del pool. Debe implementar la sincronización necesaria (ej. un `std::mutex` o una cola libre de bloqueos) si manipula estructuras compartidas. `CsvSink` incluye sincronización thread-safe internamente.
