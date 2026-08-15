#ifdef __APPLE__

#include "macos_scanner.hpp"
#include <sys/mount.h>
#include <sys/param.h>
#include <vector>
#include <string>
#include <algorithm>

namespace scanner {

void MacOSScanner::enumerate_roots(const ScanConfig& config,
                                   const std::function<void(const fs::path&)>& callback) {
    struct statfs* mounts;
    int count = getmntinfo(&mounts, MNT_NOWAIT);
    if (count <= 0) return;

    std::unordered_set<std::string> seen_devices;

    for (int i = 0; i < count; ++i) {
        const struct statfs& mnt = mounts[i];
        std::string fs_type(mnt.f_fstypename);
        std::string mount_point(mnt.f_mntonname);

        // Filtrar por tipo de filesystem (apfs, hfs, nfs, etc.)
        if (config.excluded_fs_types.find(fs_type) != config.excluded_fs_types.end()) {
            continue;
        }

        // Filtrar red
        if (!config.include_network && (fs_type == "nfs" || fs_type == "smbfs" || fs_type == "afpfs")) {
            continue;
        }

        // Evitar duplicados
        if (!seen_devices.insert(mnt.f_mntfromname).second) {
            continue;
        }

        // Excluir volúmenes de sistema si no se incluyen
        if (!config.include_system) {
            if (mount_point == "/" || mount_point == "/System" || mount_point == "/Library") {
                // Podríamos permitir /System/Volumes/Data? Depende.
                // Por defecto, excluimos el volumen raíz del sistema si include_system=false
                continue;
            }
        }

        callback(fs::path(mount_point));
    }

    // Si no se incluye sistema, agregar /Users y /Applications como raíces adicionales
    if (!config.include_system) {
        callback(fs::path("/Users"));
        callback(fs::path("/Applications"));
    } else {
        callback(fs::path("/"));
    }
}

} // namespace scanner

#endif // __APPLE__