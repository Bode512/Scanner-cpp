# 🧭 Roadmap de Desarrollo — Scanner.cpp

Este documento establece la visión del proyecto y el plan de trabajo a corto, mediano y largo plazo para **Scanner.cpp**.

---

## 🎯 Estado Actual (v1.0.x)

- [x] Motor de escaneo multi-hilo C++20 funcional con soporte `std::jthread` y `std::stop_token`.
- [x] Capa de abstracción multiplataforma (Linux, macOS, Windows).
- [x] Integración de `CsvSink` thread-safe para exportación directa a disco.
- [x] CLI funcional `scanner_cli` con opciones de timeout, workers y filtros de tamaño.
- [x] Deduplicación de hard links basada en `(device, inode)`.
- [x] Suite de pruebas unitarias y ejecutable de benchmarks.
- [x] Documentación exhaustiva, guías de contribución y automatización de CI con GitHub Actions.

---

## 🚀 Fase 1: Optimización de Rendimiento y E/S (Q3 2026)

- [ ] **Lector de Inodos Directo en Linux (`io_uring` / `statx`)**:
  - Implementar soporte opcional para `io_uring` en Linux kernels >= 5.1 para lecturas asíncronas masivas de inodos.
- [ ] **Work-Stealing Task Queue**:
  - Sustituir la cola de trabajo protegida por mutex por un planificador de robo de trabajo (*work-stealing scheduler*) para minimizar la contención de hilos en CPU con más de 32 núcleos.
- [ ] **Modo de Conteo Previo (Estimación de Progreso %)**:
  - Implementar un pase opcional ultra-rápido de recuento superficial para calcular porcentajes exactos de avance (`0% - 100%`) en la interfaz CLI.

---

## 📦 Fase 2: Nuevos Sinks y Formatos de Salida (Q4 2026)

- [ ] **Sink JSON / JSON-Lines**:
  - Soporte para exportación estructurada en `JSONL` para integración con herramientas de análisis de datos como DuckDB, Spark o Elastic.
- [ ] **Sink SQLite de Alta Velocidad**:
  - Exportación directa a base de datos relacional incrustada SQLite en transacciones por lotes (*batch commits*).
- [ ] **Sink Parquet / Arrow**:
  - Permitir la escritura directa a formato Apache Parquet para datasets de millones de archivos.

---

## 🛠️ Fase 3: Integración de Paquetes y Binding (Q1 2027)

- [ ] **Integración en Gestores de Paquetes**:
  - Añadir recetas para `vcpkg` y `Conan`.
- [ ] **Bindings para Python (pybind11 / nanobind)**:
  - Exponer la librería `scanner` como módulo Python de alto rendimiento (`pip install scanner-cpp`).
- [ ] **Soporte para Expresiones Regulares en Filtros**:
  - Añadir filtrado por patrones Glob y RegEx (`std::regex` / `std::string_view`) para nombres de archivo y extensiones.

---

## 💬 Sugerencias de la Comunidad

¿Tienes una propuesta o quieres liderar una de estas funcionalidades?
Abre un issue en la categoría [Feature Request](https://github.com/Bode512/Scanner-cpp/issues) o participa en las discusiones del proyecto.
