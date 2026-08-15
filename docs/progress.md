# 📊 Progreso y Métricas en Tiempo Real — Scanner.cpp

Este documento describe el funcionamiento de los callbacks de progreso, las métricas capturadas por `ScanProgressSnapshot` y las mejores prácticas para monitorear el estado durante el escaneo.

---

## ⚡ 1. Arquitectura de Notificación de Progreso

El motor `Scanner` expone una firma de escaneo con soporte para callbacks de progreso y tokens de cancelación:

```cpp
scanner.scan(sink, progress_callback, stop_token);
```

Cada `ScanConfig::progress_interval_ms` milisegundos (por defecto 500 ms), un hilo secundario de monitoreo genera una instantánea inmutable del estado (`ScanProgressSnapshot`) y la entrega al callback registrado.

---

## 📈 2. Métricas Expuestas en `ScanProgressSnapshot`

```cpp
struct ScanProgressSnapshot {
    uint64_t files_visited = 0;       // Total de archivos examinados hasta el momento.
    uint64_t directories_visited = 0; // Total de directorios explorados.
    uint64_t bytes_processed = 0;     // Volumen total de bytes acumulados.
    uint64_t items_emitted = 0;       // Elementos enviados exitosamente al Sink.
    uint64_t errors = 0;              // Errores de acceso/lectura no fatales interceptados.
    uint64_t skipped = 0;             // Elementos descartados por filtros configurados.
    fs::path current_path;            // Ruta del último directorio extraído de la cola.
};
```

---

## 💻 3. Ejemplo de Implementación en Consola

```cpp
auto progress = [](const ScanProgressSnapshot& snap) {
    std::cerr << "\r[Scanner Progress] "
              << "Files: " << snap.files_visited << " | "
              << "Dirs: " << snap.directories_visited << " | "
              << "Bytes: " << snap.bytes_processed << " | "
              << "Items: " << snap.items_emitted << " | "
              << "Errors: " << snap.errors
              << std::flush;
};

scanner.scan(sink, progress);
```

---

## 💡 4. Consideraciones Técnicas sobre el % Global de Avance

Los sistemas de archivos POSIX/Windows no proporcionan un contador previo de inodos globales sin realizar un escaneo completo previo.

Por lo tanto:
- `ScanProgressSnapshot` proporciona **contadores acumulativos en tiempo real**.
- No emite un porcentaje global ($0\% - 100\%$) a menos que el cliente ejecute una etapa previa de estimación o recuento rápido.
