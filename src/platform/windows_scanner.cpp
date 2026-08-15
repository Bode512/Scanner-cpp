#ifdef _WIN32

#include "windows_scanner.hpp"
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

namespace scanner {

void WindowsScanner::enumerate_roots(const ScanConfig& config,
                                     const std::function<void(const fs::path&)>& callback) {
    // Enumerar volúmenes con FindFirstVolumeW (Unicode)
    std::vector<std::wstring> volumes;
    wchar_t volume_name[MAX_PATH] = {0};
    HANDLE find = FindFirstVolumeW(volume_name, MAX_PATH);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        volumes.emplace_back(volume_name);
    } while (FindNextVolumeW(find, volume_name, MAX_PATH));
    FindVolumeClose(find);

    for (const auto& vol : volumes) {
        // Obtener el path de montaje (p.ej. C:\)
        wchar_t mount_point[MAX_PATH] = {0};
        DWORD len = 0;
        if (!GetVolumePathNamesForVolumeNameW(vol.c_str(), mount_point, MAX_PATH, &len)) {
            continue;
        }
        std::wstring mount_path(mount_point);

        // Comprobar tipo de unidad
        UINT drive_type = GetDriveTypeW(mount_path.c_str());
        bool is_removable = (drive_type == DRIVE_REMOVABLE);
        bool is_network = (drive_type == DRIVE_REMOTE);
        bool is_cdrom = (drive_type == DRIVE_CDROM);

        // Filtrar según configuración
        if (is_network && !config.include_network) continue;
        if (is_removable && !config.include_removable) continue;
        if (is_cdrom) continue; // normalmente no se escanea

        // Obtener información del volumen (filesystem, flags)
        DWORD fs_flags = 0;
        wchar_t fs_name[MAX_PATH] = {0};
        if (!GetVolumeInformationW(mount_path.c_str(), nullptr, 0, nullptr, nullptr, &fs_flags, fs_name, MAX_PATH)) {
            continue;
        }

        // Si es unidad de sistema y no se incluye sistema, excluir
        if (!config.include_system && (fs_flags & FILE_SYSTEM_ATTRS_HIDDEN)) {
            // Podríamos omitir, pero mejor usar nombre de volumen o tipo.
        }

        // Emitir raíz como volumen
        fs::path root_path(mount_path);
        callback(root_path);
    }

    // Alternativa: también escanear directorios de usuario (Documents, Desktop, etc.)?
    // La especificación pide volúmenes; el engine puede filtrar después.
}

} // namespace scanner

#endif // _WIN32