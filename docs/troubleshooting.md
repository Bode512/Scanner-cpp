# 🛠️ Guía de Solución de Problemas (Troubleshooting) — Scanner.cpp

Este documento ofrece respuestas y soluciones técnicas a los problemas y escenarios más comunes encontrados durante el uso o desarrollo con **Scanner.cpp**.

---

## ❓ 1. El escaneo parece quedarse colgado o no avanza

### Causa Potencial:
- **Contención o Bloqueo del Pool de Hilos**: Al escanear árboles con permisos extremadamente restringidos o sistemas de archivos con latencias de red elevadas (NFS, SMB), los hilos pueden bloquearse esperando respuesta de E/S del Kernel.
- **Symlinks en Bucle**: Si `follow_symlinks = true` y existen enlaces simbólicos circulares, el caminante puede quedar atrapado en un ciclo.

### Soluciones:
1. **Reducir a modo mono-hilo para depuración**:
   Ejecuta el CLI con `--workers 1` para aislar el problema:
   ```bash
   ./build/scanner_cli --workers 1 /tmp/test.csv /ruta/objetivo
   ```
2. **Desactivar symlinks**:
   Asegúrate de tener `follow_symlinks = false` en `ScanConfig` (valor por defecto).
3. **Configurar un Timeout**:
   Añade un límite de tiempo razonable en segundos usando el argumento `--timeout 30` en CLI o un `std::stop_token` con temporizador en C++.

---

## ❓ 2. Errores `Permission Denied` (Permiso Denegado) en Consola

### Causa Potencial:
Intentar escanear directorios protegidos del sistema (como `/root`, `/var/spool/rsyslog`, o carpetas de otros usuarios sin privilegios administrativos).

### Solución:
- **Resiliencia**: El motor `Scanner` captura automáticamente los errores de permisos por cada archivo o carpeta individual sin abortar el escaneo global. El contador `ScanProgressSnapshot::errors` registra estos casos.
- Si requieres acceso total al sistema de archivos en Linux, ejecuta la herramienta con elevación de privilegios:
  ```bash
  sudo ./build/scanner_cli /tmp/root_scan.csv /
  ```

---

## ❓ 3. El archivo CSV resultante está vacío o falta la cabecera

### Causa Potencial:
- La ruta especificada en `included_paths` no existe o no contiene elementos compatibles con los filtros aplicados.
- No se han otorgado permisos de escritura en la ruta de destino especificada para el CSV.

### Solución:
1. Verifica que la ruta raíz existe antes de iniciar el escaneo.
2. Revisa si se ha configurado `min_file_size` con un valor demasiado elevado que filtre la totalidad de los archivos.
3. Asegúrate de que `CsvSink` se haya cerrado o destruido adecuadamente al finalizar el escaneo (su destructor hace *flush* de los datos pendientes a disco).

---

## ❓ 4. No hay una barra de porcentaje exacto (0% - 100%) en la salida

### Explicación Técnica:
En el sistema de archivos POSIX/NTFS no es posible conocer a priori el número total exacto de archivos y subcarpetas contenidos en una jerarquía profunda sin recorrerla previamente de forma completa.

Por este motivo, **Scanner.cpp** emite contadores en tiempo real (`files_visited`, `directories_visited`, `bytes_processed`) en lugar de un porcentaje estimado que podría resultar inexacto o ralentizar el inicio del proceso.
