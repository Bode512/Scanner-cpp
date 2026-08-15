#include "scan_engine.hpp"
#include "directory_walker.hpp"
#include "size_calculator.hpp"
#include <cassert>
#include <chrono>
#include <future>
#include <memory>
#include <stack>
#include <unordered_set>

namespace scanner {

ScanEngine::ScanEngine(const ScanConfig &config)
    : config_(config), platform_(create_platform_scanner()),
      pool_(std::make_unique<ThreadPool>(config.worker_count,
                                         config.max_queue_depth)) {}

ScanEngine::~ScanEngine() { pool_->shutdown(); }

void ScanEngine::scan(
    ScanResultSink &sink,
    std::function<void(const ScanProgressSnapshot &)> progress,
    std::stop_token st) {
  // Iniciar hilo de progreso si hay callback
  std::jthread progress_thread;
  if (progress) {
    progress_thread = std::jthread([this, progress](std::stop_token pst) {
      progress_loop(pst, progress);
    });
  }

  // Enumerar raíces y lanzar una tarea por cada raíz.
  // Si el usuario especifica included_paths, esas rutas tienen prioridad
  // sobre los montajes del sistema para evitar escanear todo el FS.
  std::vector<fs::path> roots;
  if (!config_.included_paths.empty()) {
    for (const auto &inc : config_.included_paths) {
      fs::path root = fs::path(inc);
      if (!root.empty() && fs::exists(root) && fs::is_directory(root)) {
        roots.push_back(root);
      }
    }
  } else {
    platform_->enumerate_roots(
        config_, [&roots](const fs::path &root) { roots.push_back(root); });
  }

  if (roots.empty()) {
    // No hay nada que escanear
    if (progress) {
      ScanProgressSnapshot snap;
      snap.current_path = "No roots found";
      progress(snap);
    }
    return;
  }

  // Procesar cada raíz secuencialmente (podría paralelizarse, pero para evitar
  // duplicados)
  for (const auto &root : roots) {
    if (st.stop_requested())
      break;
    if (should_skip_path(root))
      continue;

    // Emitir volumen como item
    if (config_.emit_volumes) {
      ScanItem item;
      item.path = root;
      item.type = ScanItemType::Volume;
      emit_item(sink, item);
    }

    // Lanzar tarea para procesar el directorio raíz y ejecutar inline si falla
    // el encolado
    bool enqueued = pool_->enqueue(
        [this, root, &sink, st]() { process_directory(root, sink, st); }, st);
    if (!enqueued) {
      process_directory(root, sink, st);
    }
  }

  // Esperar a que todas las tareas terminen
  pool_->wait_all(st);

  // Detener hilo de progreso
  if (progress_thread.joinable()) {
    progress_thread.request_stop();
    progress_thread.join();
  }

  // Notificar finalización
  sink.on_scan_complete();
}

std::pair<uint64_t, uint64_t> ScanEngine::process_directory_internal(
    const fs::path &dir, ScanResultSink &sink, std::stop_token st) {
  if (st.stop_requested())
    return {0, 0};

  uint64_t total_logical = 0;
  uint64_t total_allocated = 0;

  std::error_code ec;
  fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied,
                            ec);
  if (ec) {
    report_error(sink, ScanError(ScanErrorCode::DirectoryNotFound, dir,
                                 "Cannot open directory", ec));
    return {0, 0};
  }

  std::vector<fs::path> subdirectories;

  for (; it != fs::directory_iterator(); it.increment(ec)) {
    if (st.stop_requested()) {
      report_error(
          sink, ScanError(ScanErrorCode::Cancelled, dir, "Scan cancelled", ec));
      return {total_logical, total_allocated};
    }
    if (ec) {
      report_error(sink, ScanError(ScanErrorCode::IoError, dir,
                                   "Directory iteration error", ec));
      break;
    }

    const fs::directory_entry &entry = *it;
    const fs::path &path = entry.path();

    if (should_skip_path(path)) {
      skipped_.fetch_add(1);
      continue;
    }

    std::error_code entry_ec;
    fs::file_status status = entry.symlink_status(entry_ec);
    if (entry_ec) {
      report_error(sink, ScanError(ScanErrorCode::IoError, path,
                                   "Cannot get status", entry_ec));
      continue;
    }

    if (fs::is_symlink(status)) {
      if (config_.emit_symlinks) {
        ScanItem item;
        item.path = path;
        item.type = ScanItemType::Symlink;
        item.is_symlink = true;
        std::error_code target_ec;
        item.symlink_target = fs::read_symlink(path, target_ec);
        if (target_ec)
          item.error_message = "Broken symlink";
        emit_item(sink, item);
      }
      continue;
    }

    if (fs::is_directory(status)) {
      subdirectories.push_back(path);
      ++directories_visited_;
      continue;
    }

    if (fs::is_regular_file(status)) {
      ++files_visited_;
      SizeCalculator calc(config_);
      auto result = calc.get_file_info(path);
      if (result.has_value()) {
        const FileSizeInfo &info = result.value();
        total_logical += info.logical_size;
        total_allocated += info.allocated_size;
        bytes_processed_.fetch_add(info.logical_size);

        if (config_.count_hardlinks_once && info.identity.valid &&
            info.hardlink_count > 1) {
          if (check_hardlink(info.identity)) {
            skipped_.fetch_add(1);
            continue;
          }
        }

        if (config_.emit_files) {
          ScanItem item;
          item.path = path;
          item.type = ScanItemType::File;
          item.logical_size = info.logical_size;
          item.allocated_size = info.allocated_size;
          item.identity = info.identity;
          item.hardlink_count = info.hardlink_count;
          if (config_.min_file_size == 0 ||
              info.logical_size >= config_.min_file_size) {
            emit_item(sink, item);
          }
        }
      } else {
        report_error(sink, result.error());
      }
    }
  }

  // Encolar subdirectorios y recoger futuros
  std::vector<std::future<std::pair<uint64_t, uint64_t>>> futures;
  for (const auto &subdir : subdirectories) {
    if (st.stop_requested())
      break;

    auto task_ptr =
        std::make_shared<std::packaged_task<std::pair<uint64_t, uint64_t>()>>(
            [this, subdir, &sink, st]() {
              return process_directory_internal(subdir, sink, st);
            });

    futures.push_back(task_ptr->get_future());

    bool enqueued = pool_->enqueue([task_ptr]() mutable { (*task_ptr)(); }, st);
    if (!enqueued) {
      // ejecutar sincronamente si no se pudo encolar
      (*task_ptr)();
    }
  }

  for (auto &fut : futures) {
    try {
      while (fut.wait_for(std::chrono::milliseconds(0)) !=
             std::future_status::ready) {
        if (st.stop_requested()) {
          break;
        }
        if (!pool_->run_one_task()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
      }
      if (st.stop_requested())
        break;

      auto res = fut.get();
      total_logical += res.first;
      total_allocated += res.second;
    } catch (const std::exception &e) {
      // convertir en ScanError genérico
      report_error(
          sink, ScanError(ScanErrorCode::IoError, dir,
                          std::string("Subdirectory task failed: ") + e.what(),
                          std::error_code()));
    }
  }

  if (config_.emit_directories) {
    ScanItem item;
    item.path = dir;
    item.type = ScanItemType::Directory;
    item.logical_size = total_logical;
    item.allocated_size = total_allocated;
    emit_item(sink, item);
  }

  return {total_logical, total_allocated};
}

void ScanEngine::process_directory(const fs::path &dir, ScanResultSink &sink,
                                   std::stop_token st) {
  // Wrapper que invoca la implementación interna que devuelve totals.
  process_directory_internal(dir, sink, st);
}

void ScanEngine::process_file(const fs::path &file, ScanResultSink &sink,
                              std::stop_token st) {
  // No se usa directamente; se maneja en process_directory
}

void ScanEngine::process_symlink(const fs::path &link, ScanResultSink &sink,
                                 std::stop_token st) {
  // No se usa directamente
}

bool ScanEngine::should_skip_path(const fs::path &path) const {
  // Comprobar excluded_paths y included_paths
  const std::string path_str = path.string();
  for (const auto &excl : config_.excluded_paths) {
    if (path_str.find(excl) == 0)
      return true;
  }
  if (!config_.included_paths.empty()) {
    bool included = false;
    for (const auto &inc : config_.included_paths) {
      if (path_str.find(inc) == 0) {
        included = true;
        break;
      }
    }
    if (!included)
      return true;
  }
  return false;
}

bool ScanEngine::check_hardlink(const FileIdentity &id) {
  uint64_t key = id.device ^ (id.inode << 32) ^ (id.inode >> 32);
  std::lock_guard lock(hardlink_mutex_);
  return !hardlink_seen_.insert(key).second;
}

void ScanEngine::emit_item(ScanResultSink &sink, ScanItem item) {
  items_emitted_.fetch_add(1);
  sink.on_item(item);
}

void ScanEngine::report_error(ScanResultSink &sink, ScanError error) {
  errors_.fetch_add(1);
  sink.on_error(error);
}

void ScanEngine::progress_loop(
    std::stop_token st,
    std::function<void(const ScanProgressSnapshot &)> callback) {
  using namespace std::chrono_literals;
  while (!st.stop_requested()) {
    std::this_thread::sleep_for(
        std::chrono::milliseconds(config_.progress_interval_ms));
    if (st.stop_requested())
      break;
    ScanProgressSnapshot snap;
    snap.files_visited = files_visited_.load();
    snap.directories_visited = directories_visited_.load();
    snap.bytes_processed = bytes_processed_.load();
    snap.items_emitted = items_emitted_.load();
    snap.errors = errors_.load();
    snap.skipped = skipped_.load();
    // current_path no lo implementamos completamente
    callback(snap);
  }
}

} // namespace scanner