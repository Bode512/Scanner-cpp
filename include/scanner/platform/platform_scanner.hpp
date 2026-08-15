#pragma once

#include "../scan_types.hpp"
#include "../scan_config.hpp"
#include <filesystem>
#include <functional>

namespace scanner {

// Interfaz para enumerar las raíces a escanear.
class PlatformScanner {
public:
    virtual ~PlatformScanner() = default;

    // Llama al callback con cada raíz (volumen o directorio principal).
    // Puede ser costoso (p.ej. enumerar unidades en Windows, leer /proc/mounts en Linux).
    virtual void enumerate_roots(const ScanConfig& config,
                                 const std::function<void(const fs::path&)>& callback) = 0;
};

// Factory function para crear el backend según plataforma.
std::unique_ptr<PlatformScanner> create_platform_scanner();

} // namespace scanner