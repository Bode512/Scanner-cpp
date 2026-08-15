# ADR 0001: Motor de Escaneo Multi-hilo basado en C++20 `std::jthread` y `std::stop_token`

* **Estado**: Aceptado
* **Fecha**: 2026-08-16
* **Autores**: Equipo Mantenedor de Scanner.cpp

---

##  Contexto y Problema

El escaneo de sistemas de archivos de gran tamaño (decenas de millones de archivos en discos SSD NVMe de alta velocidad) mediante algoritmos recursivos mono-hilo tradicionales sufre de latencias causadas por las llamadas al sistema POSIX/Win32 (`readdir`, `stat`, `GetFileInformationByHandleEx`).

Se requería diseñar una arquitectura de escaneo capaz de:
1. Escalar de forma lineal con el número de núcleos de CPU disponibles.
2. Evitar desbordamientos de pila (*stack overflow*) provocados por la recursión profunda de carpetas.
3. Permitir la cancelación inmediata y segura (*thread-safe*) sin fugas de memoria o descriptores de archivo abiertos.

---

##  Decisión Tomada

Decidimos implementar el motor de escaneo utilizando un modelo de **Pool de Hilos Dinámico** basado en las características nativas de **C++20**:

1. **Uso de `std::jthread`**: RAII implícito para la gestión de hilos, asegurando un *join* automático durante la destrucción del objeto.
2. **Cancelación mediante `std::stop_token`**: Transmisión cooperativa de solicitudes de parada a todos los hilos de trabajo sin requerir banderas atómicas manuales ni señales propensas a errores.
3. **Cola de Tareas Thread-Safe (`ThreadPool`)**: Las rutas de directorios por explorar se encolan como tareas independientes procesadas concurrentemente por los trabajadores.

---

## ⚖️ Consecuencias

### Positivas:
- **Rendimiento**: Multiplica por $N$ el rendimiento de escaneo en almacenamiento rápido (SSD NVMe / arreglos RAID).
- **Seguridad**: Ausencia de fugas de recursos gracias a RAII y `std::jthread`.
- **Portabilidad**: Código C++20 estándar ejecutable sin dependencias externas pesadas (como Boost.Thread).

### Negativas / Desafíos:
- **Orden de Salida**: La emisión de items en el `ScanResultSink` no es determinista en orden debido a la concurrencia.
- **Sincronización en Sinks**: Requiere que cualquier implementación de `ScanResultSink` sea internamente thread-safe.
