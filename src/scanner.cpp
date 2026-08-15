#include "scanner.hpp"
#include "scan_engine.hpp"

namespace scanner {

Scanner::Scanner(ScanConfig config) : config_(std::move(config)) {}

Scanner::~Scanner() = default;

void Scanner::scan(ScanResultSink& sink,
                   std::function<void(const ScanProgressSnapshot&)> progress,
                   std::stop_token st) {
    ScanEngine engine(config_);
    engine.scan(sink, std::move(progress), st);
}

} // namespace scanner