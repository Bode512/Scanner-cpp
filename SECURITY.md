# Política de Seguridad de Scanner.cpp

En **Scanner.cpp** consideramos la seguridad y la integridad del software como aspectos fundamentales. Agradecemos la colaboración de la comunidad de investigación en seguridad para reportar vulnerabilidades de manera responsable.

---

## 🛡️ Versiones Soportadas

Actualmente, las siguientes versiones de **Scanner.cpp** reciben parches y actualizaciones de seguridad:

| Versión | Soportada | Estado |
| :--- | :---: | :--- |
| `1.0.x` | ✅ | Versión estable principal (C++20) |
| `< 1.0.0` | ❌ | Versión de desarrollo anterior |

---

## 🚨 Reportar una Vulnerabilidad

**Por favor, NO reportes vulnerabilidades de seguridad a través de Issues públicos de GitHub.**

Para reportar una vulnerabilidad o un fallo potencial de seguridad:

1. Utiliza la funcionalidad de **Vulnerability Reporting** nativa de GitHub en la pestaña `Security -> Advisory -> Report a vulnerability` de este repositorio.
2. Si la opción nativa no está disponible, contacta directamente con el mantenedor principal del repositorio a través de su perfil oficial de GitHub (`@Bode512`).

### Información deseada en el reporte

Para ayudarnos a evaluar y resolver la vulnerabilidad rápidamente, por favor incluye:

- **Descripción**: Breve resumen de la vulnerabilidad y su impacto potencial (p. ej. desbordamiento de búfer, consumo excesivo de memoria/DoS, recorrido no autorizado de rutas).
- **Entorno**: Sistema operativo (Linux, macOS, Windows), versión del compilador y arquitectura.
- **PoC (Proof of Concept)**: Pasos detallados o código de ejemplo mínimo para reproducir el fallo.
- **Impacto**: Su evaluación del impacto (ej. confidencialidad, integridad, denegación de servicio).

---

## ⏱️ Proceso y Tiempos de Respuesta

- **Confirmación de recepción**: Responderemos a los reportes de seguridad en un plazo máximo de **48 horas**.
- **Evaluación**: Evaluaremos el impacto y gravedad de la vulnerabilidad en un plazo de **5 días hábiles**.
- **Solución y Divulgación**: Trabajaremos en un parche de corrección. Una vez verificado, publicaremos una versión de parche (`1.0.x`) y acreditaremos adecuadamente al investigador.

---

## 🔒 Mejores Prácticas Recomendadas para los Usuarios

- **Privilegios de Usuario**: Al escanear directorios del sistema en Linux (`/proc`, `/sys`) o Windows (`System32`), ejecuta el escáner con los permisos mínimos necesarios.
- **Symlinks**: Ten precaución al habilitar `follow_symlinks = true` en estructuras de archivos no confiables para evitar bucles o acceso a rutas fuera del scope deseado.
