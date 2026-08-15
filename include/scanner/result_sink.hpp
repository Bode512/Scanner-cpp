#pragma once

#include "scan_types.hpp"

namespace scanner {

class ScanResultSink {
public:
    virtual ~ScanResultSink() = default;

    // Se llama por cada item descubierto y medido.
    // Puede ser llamado desde múltiples threads; implementar thread-safe.
    virtual void on_item(const ScanItem& item) = 0;

    // Se llama cuando ocurre un error no fatal.
    virtual void on_error(const ScanError& error) = 0;

    // Opcional: se llama al finalizar el escaneo de cada raíz.
    virtual void on_root_complete(const fs::path& root) { (void)root; }

    // Opcional: se llama al completar todo el escaneo.
    virtual void on_scan_complete() {}
};

} // namespace scanner