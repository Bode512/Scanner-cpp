#include "directory_walker.hpp"
#include <stack>
#include <unordered_set>
#include <system_error>

namespace scanner {

void DirectoryWalker::walk(const fs::path& root, const EntryCallback& callback, std::stop_token st) {
    walk_internal(root, callback, st);
}

void DirectoryWalker::walk_internal(const fs::path& root, const EntryCallback& callback, std::stop_token st) {
    std::stack<fs::path> dirs_to_visit;
    std::unordered_set<fs::path> visited_symlink_dirs; // Para evitar ciclos si seguimos symlinks

    dirs_to_visit.push(root);

    while (!dirs_to_visit.empty() && !st.stop_requested()) {
        fs::path current = dirs_to_visit.top();
        dirs_to_visit.pop();

        std::error_code ec;
        fs::directory_iterator it(current, fs::directory_options::skip_permission_denied, ec);
        if (ec) {
            // Reportar error? El engine lo maneja.
            continue;
        }

        for (; it != fs::directory_iterator(); it.increment(ec)) {
            if (st.stop_requested()) break;
            if (ec) break;

            const fs::directory_entry& entry = *it;
            callback(entry); // El engine decide qué hacer con la entrada

            if (entry.is_directory(ec)) {
                if (!config_.follow_symlinks && entry.is_symlink(ec)) {
                    // Si es symlink a directorio y no seguimos, no recorrer
                    continue;
                }
                if (config_.follow_symlinks && entry.is_symlink(ec)) {
                    // Si seguimos symlinks, verificar ciclo
                    fs::path canonical = fs::weakly_canonical(entry.path(), ec);
                    if (ec) continue;
                    if (!visited_symlink_dirs.insert(canonical).second) {
                        continue; // ciclo detectado
                    }
                }
                dirs_to_visit.push(entry.path());
            }
        }
    }
}

} // namespace scanner