#ifdef __linux__

#include "linux_scanner.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <algorithm>

namespace scanner {

void LinuxScanner::enumerate_roots(const ScanConfig& config,
                                   const std::function<void(const fs::path&)>& callback) {
    // Leer /proc/mounts para obtener montajes
    std::ifstream mounts("/proc/mounts");
    std::string line;
    std::unordered_set<std::string> seen_devices;

    while (std::getline(mounts, line)) {
        std::istringstream iss(line);
        std::string device, mount_point, fs_type, options;
        iss >> device >> mount_point >> fs_type >> options;

        // Filtrar por tipo de filesystem
        if (config.excluded_fs_types.find(fs_type) != config.excluded_fs_types.end()) {
            continue;
        }

        // Filtrar filesystems de red (NFS, CIFS) si no se incluyen
        if (!config.include_network) {
            if (fs_type == "nfs" || fs_type == "nfs4" || fs_type == "cifs" || fs_type == "smbfs" || fs_type == "fuse.sshfs") {
                continue;
            }
        }

        // Evitar duplicados (mismo dispositivo montado en varios puntos)
        if (!device.empty() && device != "none" && !seen_devices.insert(device).second) {
            continue;
        }

        // Excluir montajes virtuales y pseudo-filesystems ya filtrados por excluded_fs_types
        // También excluir /proc, /sys, /dev si no se incluyen system
        if (!config.include_system) {
            if (mount_point == "/proc" || mount_point == "/sys" || mount_point == "/dev" || mount_point == "/run" || mount_point == "/tmp") {
                continue;
            }
        }

        // Emitir raíz
        callback(fs::path(mount_point));
    }
}

} // namespace scanner

#endif // __linux__