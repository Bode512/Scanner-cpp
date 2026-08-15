#include "platform_scanner.hpp"

#if defined(_WIN32)
#include "windows_scanner.hpp"
#elif defined(__APPLE__)
#include "macos_scanner.hpp"
#elif defined(__linux__)
#include "linux_scanner.hpp"
#endif

namespace scanner {

std::unique_ptr<PlatformScanner> create_platform_scanner() {
#if defined(_WIN32)
    return std::make_unique<WindowsScanner>();
#elif defined(__APPLE__)
    return std::make_unique<MacOSScanner>();
#elif defined(__linux__)
    return std::make_unique<LinuxScanner>();
#else
    return nullptr;
#endif
}

} // namespace scanner
