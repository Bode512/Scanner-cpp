#pragma once
#include "platform_scanner.hpp"

namespace scanner {

class MacOSScanner : public PlatformScanner {
public:
    void enumerate_roots(const ScanConfig& config,
                         const std::function<void(const fs::path&)>& callback) override;
};

} // namespace scanner