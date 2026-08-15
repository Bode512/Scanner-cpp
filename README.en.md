<div align="center">

# ⚡ Scanner.cpp

### *High-Performance Multi-Threaded Filesystem Scanner in C++20*

[![C++20 Standard](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)](https://en.cppreference.com/w/cpp/compiler_support/20)
[![CMake Build](https://img.shields.io/badge/CMake-3.15%2B-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org)
[![License: Apache 2.0](https://img.shields.io/badge/License-Apache_2.0-D22128?style=for-the-badge&logo=apache&logoColor=white)](LICENSE)
[![Platforms: Linux | macOS | Windows](https://img.shields.io/badge/Platforms-Linux%20%7C%20macOS%20%7C%20Windows-222222?style=for-the-badge&logo=linux&logoColor=white)](docs/architecture.md)
[![CI Build](https://img.shields.io/badge/CI-Passing-2ea44f?style=for-the-badge&logo=githubactions&logoColor=white)](.github/workflows/ci.yml)

<br />

**[ 📖 Documentation ](docs/getting-started.md)** &nbsp;|&nbsp;
**[ 🚀 Quick Start ](#-quick-start)** &nbsp;|&nbsp;
**[ ⚙️ Architecture ](docs/architecture.md)** &nbsp;|&nbsp;
**[ 📚 API Reference ](docs/api.md)** &nbsp;|&nbsp;
**[ 🤝 Contributing ](CONTRIBUTING.md)** &nbsp;|&nbsp;
**[ 🇪🇸 Versión en Español ](README.md)**

---

</div>

## 📌 Overview

**Scanner.cpp** is a modern, ultra-high-performance C++20 library and interactive command-line tool (`scanner_cli`). Designed for high-speed recursive traversal and analysis of massive directory hierarchies and filesystems, it accurately measures both **logical file size** and **physical allocated disk usage**, de-duplicates hard links, and streams results in real-time (`ScanResultSink`) without exhausting system memory.

It leverages modern C++20 primitives such as `std::jthread` and `std::stop_token`, combined with platform-native OS APIs on Linux (`statx`/`fts`), macOS (`getattrlist`), and Windows (`GetFileInformationByHandleEx`), maximizing throughput on modern NVMe SSDs and RAID arrays.

---

## ✨ Key Features

| Feature | Description |
| :--- | :--- |
| 🚀 **Scalable Concurrency** | Dynamic multi-core `ThreadPool` with thread-safe work distribution. |
| ⚡ **Native POSIX / Win32 Abstraction** | Platform-optimized low-level OS calls (Linux `statx`, macOS `getattrlist`, Windows Win32 API). |
| 📊 **Streaming Sink Architecture** | Continuous results emission (`ScanResultSink` / `CsvSink`) with flat $\mathcal{O}(1)$ memory footprint. |
| 🔗 **Hard Link Deduplication** | Unique inode tracking using `(device, inode)` pairs to prevent double-counting physical allocated space. |
| 🎯 **Dual Size Metrics** | Differential measurement between nominal file size and actual allocated disk blocks. |
| 🛑 **Cooperative Cancellation & Telemetry** | Clean shutdown via C++20 `std::stop_token` and live progress snapshots (`ScanProgressSnapshot`). |
| 🛠️ **Advanced Filters** | Automatic exclusion of virtual filesystems (`/proc`, `/sys`), hidden files, depth limits, and size filters. |

---

## 🚀 Quick Start

### 1. Build in 60 seconds

```bash
# 1. Clone repository
git clone https://github.com/Bode512/Scanner-cpp.git
cd Scanner-cpp

# 2. Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 3. Run test suite
ctest --test-dir build --output-on-failure
```

---

### 2. Command-Line Tool (`scanner_cli`)

Scan any directory and export results directly to CSV:

```bash
# Scan /var/log using 8 worker threads
./build/scanner_cli /tmp/var_log_scan.csv /var/log --workers 8
```

---

### 3. C++20 API Example

```cpp
#include "scanner/scanner.hpp"
#include "integration/csv_sink.hpp"
#include <iostream>

using namespace scanner;

int main() {
    ScanConfig config;
    config.included_paths.push_back("/home/user/Documents");
    config.worker_count = std::thread::hardware_concurrency();

    CsvSink sink("documents_report.csv");
    Scanner scanner_engine(config);
    scanner_engine.scan(sink);

    std::cout << "Scan completed successfully.\n";
    return 0;
}
```

---

## 📜 License

Distributed under the **[Apache 2.0 License](LICENSE)**.
