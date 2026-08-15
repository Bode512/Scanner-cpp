# Registro de Cambios (Changelog)

Todos los cambios notables en este proyecto serán documentados en este archivo.

El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.0.0/), y este proyecto adhiere a [Semantic Versioning](https://semver.org/lang/es/).

---

## [Unreleased]

### Added
- Plantillas nativas de GitHub para reporte de errores (`bug_report.yml`) y solicitudes de funcionalidades (`feature_request.yml`).
- Flujo de Integración Continua (CI) multi-plataforma en GitHub Actions para Linux, macOS y Windows.
- Documentación completa en `docs/` sobre arquitectura, guía de inicio rápido, referencia API y configuración.
- Archivo `ROADMAP.md` para el seguimiento del desarrollo futuro.
- Configuración de `SECURITY.md`, `CODE_OF_CONDUCT.md` y `CONTRIBUTING.md`.

---

## [1.0.0] - 2026-08-16

### Added
- **Motor de Escaneo Multi-hilo C++20**:
  - Soporte para concurrencia configurable mediante `std::jthread` y `std::stop_token`.
  - Abstracciones de plataforma nativas (`Linux`, `macOS`, `Windows`) para captura eficiente de metadatos de archivos.
  - Cálculo diferencial de tamaño lógico vs. tamaño físico asignado en disco.
  - Deduplicación de hard links utilizando identidades `(device, inode)`.
- **CLI (`scanner_cli`)**:
  - Interfaz de línea de comandos para escaneo de directorios directos con exportación a CSV.
  - Opciones para ajustar trabajadores (`--workers`), filtrado de tamaño (`--min-size`), omisión de archivos/carpetas y timeouts (`--timeout`).
- **Sink de Integración CSV (`CsvSink`)**:
  - Exportador thread-safe a formato CSV con cabeceras estándar.
- **Soporte para Callbacks de Progreso**:
  - Captura en tiempo real de instantáneas de progreso (`ScanProgressSnapshot`).
- **Suite de Pruebas y Benchmarking**:
  - Ejecutable de prueba unitaria `test_scanner` y runner de rendimiento `benchmark`.
