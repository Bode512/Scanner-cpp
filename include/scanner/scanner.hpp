#pragma once

#include "scan_config.hpp"
#include "scan_types.hpp"
#include "result_sink.hpp"
#include <functional>
#include <stop_token>

namespace scanner {

class Scanner {
public:
    explicit Scanner(ScanConfig config);
    ~Scanner();

    // Ejecuta el escaneo. Devuelve void. La cancelación se maneja con stop_token.
    void scan(ScanResultSink& sink,
              std::function<void(const ScanProgressSnapshot&)> progress = nullptr,
              std::stop_token st = std::stop_token{});

private:
    ScanConfig config_;
};

} // namespace scanner