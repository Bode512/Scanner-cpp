#include "size_calculator.hpp"

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__)
#include <sys/stat.h>
#include <sys/statvfs.h>
#elif defined(__APPLE__)
#include <sys/stat.h>
#include <sys/attr.h>
#endif

namespace scanner {

ScanResult<FileSizeInfo> SizeCalculator::get_file_info(const fs::path& path) const {
    std::error_code ec;
    fs::file_status status = fs::symlink_status(path, ec);
    if (ec) {
        return ScanError(ScanErrorCode::InvalidPath, path, "Cannot get status", ec);
    }

    if (fs::is_symlink(status)) {
        FileSizeInfo info;
        info.is_symlink = true;
        std::error_code target_ec;
        info.symlink_target = fs::read_symlink(path, target_ec);
        if (target_ec) {
            return ScanError(ScanErrorCode::BrokenSymlink, path, "Broken symlink", target_ec);
        }
        return info;
    }

    if (!fs::is_regular_file(status)) {
        return ScanError(ScanErrorCode::UnsupportedFilesystem, path, "Not a regular file");
    }

    FileSizeInfo info;
    info.logical_size = 0;
    info.allocated_size = 0;
    info.hardlink_count = 0;

#ifdef _WIN32
    // Windows: usar CreateFileW y GetFileInformationByHandleEx
    HANDLE h = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        if (err == ERROR_ACCESS_DENIED) {
            return ScanError(ScanErrorCode::PermissionDenied, path, "Access denied");
        }
        return ScanError(ScanErrorCode::IoError, path, "CreateFile failed", std::error_code(err, std::system_category()));
    }

    BY_HANDLE_FILE_INFORMATION file_info;
    if (!GetFileInformationByHandle(h, &file_info)) {
        DWORD err = GetLastError();
        CloseHandle(h);
        return ScanError(ScanErrorCode::IoError, path, "GetFileInformationByHandle failed", std::error_code(err, std::system_category()));
    }

    LARGE_INTEGER size;
    size.HighPart = file_info.nFileSizeHigh;
    size.LowPart = file_info.nFileSizeLow;
    info.logical_size = size.QuadPart;

    // Obtener tamaño asignado con FileAllocationInfo
    FILE_ALLOCATION_INFO alloc_info;
    if (GetFileInformationByHandleEx(h, FileAllocationInfo, &alloc_info, sizeof(alloc_info))) {
        info.allocated_size = static_cast<uint64_t>(alloc_info.AllocationSize.QuadPart);
    } else {
        // Fallback: usar número de clusters? Simplificamos.
        info.allocated_size = info.logical_size; // No exacto, pero no inventamos
    }

    // Identidad para hard links
    info.identity.device = file_info.dwVolumeSerialNumber;
    info.identity.inode = (static_cast<uint64_t>(file_info.nFileIndexHigh) << 32) | file_info.nFileIndexLow;
    info.identity.valid = true;

    // Hardlink count
    info.hardlink_count = file_info.nNumberOfLinks;

    CloseHandle(h);
#elif defined(__linux__) || defined(__APPLE__)
    struct stat st;
    if (::lstat(path.c_str(), &st) != 0) {
        int err = errno;
        if (err == EACCES) {
            return ScanError(ScanErrorCode::PermissionDenied, path, "Access denied");
        }
        if (err == ENOENT) {
            return ScanError(ScanErrorCode::FileNotFound, path, "File not found");
        }
        return ScanError(ScanErrorCode::IoError, path, "stat failed", std::error_code(err, std::generic_category()));
    }

    info.logical_size = static_cast<uint64_t>(st.st_size);
    info.allocated_size = static_cast<uint64_t>(st.st_blocks) * 512;
    info.identity.device = st.st_dev;
    info.identity.inode = st.st_ino;
    info.identity.valid = true;
    info.hardlink_count = st.st_nlink;
#endif

    return info;
}

ScanResult<FileIdentity> SizeCalculator::get_directory_identity(const fs::path& path) const {
    std::error_code ec;
    fs::file_status status = fs::symlink_status(path, ec);
    if (ec || !fs::is_directory(status)) {
        return ScanError(ScanErrorCode::DirectoryNotFound, path, "Not a directory");
    }

    FileIdentity id;
#ifdef _WIN32
    // Similar: abrir con CreateFileW y obtener información
    HANDLE h = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return ScanError(ScanErrorCode::IoError, path, "Cannot open directory");
    BY_HANDLE_FILE_INFORMATION fi;
    if (GetFileInformationByHandle(h, &fi)) {
        id.device = fi.dwVolumeSerialNumber;
        id.inode = (static_cast<uint64_t>(fi.nFileIndexHigh) << 32) | fi.nFileIndexLow;
        id.valid = true;
    }
    CloseHandle(h);
#else
    struct stat st;
    if (::lstat(path.c_str(), &st) == 0) {
        id.device = st.st_dev;
        id.inode = st.st_ino;
        id.valid = true;
    }
#endif
    if (!id.valid) return ScanError(ScanErrorCode::IoError, path, "Cannot get directory identity");
    return id;
}

} // namespace scanner