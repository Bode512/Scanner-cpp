#!/usr/bin/env python3
"""
End-to-End Stress & Validation Test for Storage Scanner.

This script performs a comprehensive, real-world test of the scanner
on specified directories, validates CSV output, monitors system resources,
and generates detailed artifacts including logs, metrics, and an HTML report.

Usage:
    python3 e2e_test.py --paths /home/bode /home/user --output-dir ./test_results
"""

import argparse
import csv
import json
import logging
import os
import platform
import signal
import subprocess
import sys
import threading
import time
from dataclasses import dataclass, asdict, field
from datetime import datetime, timezone
from pathlib import Path
from typing import List, Dict, Optional, Tuple, Any
from concurrent.futures import ThreadPoolExecutor, as_completed

# Third-party (install with: pip install psutil)
try:
    import psutil
except ImportError:
    print("ERROR: 'psutil' is required. Install with 'pip install psutil'", file=sys.stderr)
    sys.exit(1)

# -----------------------------------------------------------------------------
# Configuration & Constants
# -----------------------------------------------------------------------------
DEFAULT_TIMEOUT_SCAN_SEC = 600  # 10 minutes per scan
DEFAULT_STRESS_REPETITIONS = 2
DEFAULT_STRESS_PARALLELISM = 1
DEFAULT_WORKERS = 0  # auto
PROGRESS_INTERVAL_SEC = 0.5

# -----------------------------------------------------------------------------
# Data Models
# -----------------------------------------------------------------------------
@dataclass
class PreflightResult:
    os_name: str
    python_version: str
    psutil_version: str
    scanner_binary: str
    paths: List[str]
    paths_exist: Dict[str, bool]
    paths_readable: Dict[str, bool]
    disk_space: Dict[str, int]
    warnings: List[str] = field(default_factory=list)

@dataclass
class DiscoveryStats:
    path: str
    total_files: int = 0
    total_dirs: int = 0
    total_symlinks: int = 0
    total_bytes: int = 0
    inaccessible_entries: int = 0
    max_depth_reached: int = 0

@dataclass
class ScanRunResult:
    run_id: str
    timestamp_start: str
    timestamp_end: str
    duration_sec: float
    exit_code: int
    timeout_hit: bool
    stdout: str = ""
    stderr: str = ""
    metrics: List[Dict[str, Any]] = field(default_factory=list)  # periodic samples
    peak_rss_mb: float = 0.0
    avg_cpu_percent: float = 0.0
    avg_io_read_mb: float = 0.0
    avg_io_write_mb: float = 0.0
    csv_path: str = ""
    csv_size_bytes: int = 0

@dataclass
class CSVValidationResult:
    exists: bool = False
    size_bytes: int = 0
    row_count: int = 0
    header: List[str] = field(default_factory=list)
    expected_columns: List[str] = field(default_factory=list)
    missing_columns: List[str] = field(default_factory=list)
    duplicate_rows: int = 0
    null_or_empty_count: int = 0
    invalid_type_count: int = 0
    inconsistent_rows: int = 0
    read_ok: bool = False
    encoding_issues: List[str] = field(default_factory=list)
    delimiter: str = ","
    warnings: List[str] = field(default_factory=list)

@dataclass
class ErrorRecord:
    phase: str
    severity: str  # CRITICAL, ERROR, WARNING, INFO
    message: str
    timestamp: str
    details: str = ""

@dataclass
class FinalReport:
    status: str  # PASS, PASS_WITH_WARNINGS, FAIL, ERROR
    summary: Dict[str, Any]
    phases: Dict[str, Any]
    errors: List[ErrorRecord] = field(default_factory=list)
    artifacts: Dict[str, str] = field(default_factory=dict)

# -----------------------------------------------------------------------------
# Utility functions
# -----------------------------------------------------------------------------
def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()

def format_bytes(n: int) -> str:
    for unit in ['B','KB','MB','GB','TB']:
        if abs(n) < 1024.0:
            return f"{n:.2f} {unit}"
        n /= 1024.0
    return f"{n:.2f} PB"

def run_command(cmd: List[str], timeout: int = 60) -> Tuple[int, str, str]:
    """Run command and capture output with timeout."""
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        return proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired as e:
        return -1, "", f"Timeout after {timeout}s: {e}"

# -----------------------------------------------------------------------------
# Logging setup
# -----------------------------------------------------------------------------
class Logger:
    def __init__(self, log_file: Path):
        self.log_file = log_file
        self.logger = logging.getLogger('e2e_test')
        self.logger.setLevel(logging.DEBUG)
        formatter = logging.Formatter('%(asctime)s | %(levelname)s | %(message)s')
        file_handler = logging.FileHandler(log_file, mode='w')
        file_handler.setFormatter(formatter)
        console_handler = logging.StreamHandler(sys.stdout)
        console_handler.setFormatter(formatter)
        self.logger.addHandler(file_handler)
        self.logger.addHandler(console_handler)

    def info(self, msg): self.logger.info(msg)
    def warning(self, msg): self.logger.warning(msg)
    def error(self, msg): self.logger.error(msg)
    def debug(self, msg): self.logger.debug(msg)

# -----------------------------------------------------------------------------
# Resource Monitor (using psutil)
# -----------------------------------------------------------------------------
class ResourceMonitor:
    """Samples CPU, RAM, IO for a given process periodically."""
    def __init__(self, pid: int, interval: float = 1.0):
        self.pid = pid
        self.interval = interval
        self.samples: List[Dict[str, Any]] = []
        self._stop = threading.Event()
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._proc = psutil.Process(pid) if psutil.pid_exists(pid) else None

    def start(self):
        self._thread.start()

    def stop(self):
        self._stop.set()
        self._thread.join(timeout=2)
        return self.samples

    def _run(self):
        while not self._stop.is_set():
            if self._proc and psutil.pid_exists(self.pid):
                try:
                    with self._proc.oneshot():
                        cpu = self._proc.cpu_percent(interval=None)
                        rss = self._proc.memory_info().rss / (1024 * 1024)
                        io = self._proc.io_counters() if hasattr(self._proc, 'io_counters') else None
                        read_mb = io.read_bytes / (1024 * 1024) if io else 0.0
                        write_mb = io.write_bytes / (1024 * 1024) if io else 0.0
                        sample = {
                            'timestamp': utc_now(),
                            'cpu_percent': cpu,
                            'rss_mb': rss,
                            'io_read_mb': read_mb,
                            'io_write_mb': write_mb,
                        }
                        self.samples.append(sample)
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    break
            time.sleep(self.interval)

# -----------------------------------------------------------------------------
# Phase implementations
# -----------------------------------------------------------------------------
class E2ETest:
    def __init__(self, args):
        self.args = args
        self.output_dir = Path(args.output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.logger = Logger(self.output_dir / 'test.log')
        self.errors: List[ErrorRecord] = []
        self.report: FinalReport = None
        self.preflight: PreflightResult = None
        self.discovery: Dict[str, DiscoveryStats] = {}
        self.scan_results: List[ScanRunResult] = []
        self.csv_validation: CSVValidationResult = None

        # Paths
        self.scanner_binary = args.scanner_binary or self._find_scanner_binary()

    def _find_scanner_binary(self) -> str:
        # Assume it's in build/ directory relative to script
        candidates = [
            Path(__file__).parent.parent / 'build' / 'scanner_cli',
            Path(__file__).parent.parent / 'scanner_cli',
            Path.cwd() / 'scanner_cli'
        ]
        for c in candidates:
            if c.exists() and c.is_file():
                return str(c)
        return 'scanner_cli'  # fallback to PATH

    def add_error(self, phase: str, severity: str, message: str, details: str = ""):
        err = ErrorRecord(phase=phase, severity=severity, message=message,
                          timestamp=utc_now(), details=details)
        self.errors.append(err)
        if severity == 'CRITICAL':
            self.logger.error(f"[{phase}] {message} {details}")
        elif severity == 'ERROR':
            self.logger.error(f"[{phase}] {message} {details}")
        elif severity == 'WARNING':
            self.logger.warning(f"[{phase}] {message} {details}")
        else:
            self.logger.info(f"[{phase}] {message} {details}")

    # -------------------------------------------------------------------------
    def phase_preflight(self):
        self.logger.info("=== Phase 1: Preflight ===")
        paths = self.args.paths
        pre = PreflightResult(
            os_name=platform.system(),
            python_version=sys.version.split()[0],
            psutil_version=psutil.__version__,
            scanner_binary=self.scanner_binary,
            paths=paths,
            paths_exist={},
            paths_readable={},
            disk_space={},
        )
        # Check binary exists
        if not Path(self.scanner_binary).exists():
            # If not absolute, check PATH
            if not self._command_exists(self.scanner_binary):
                self.add_error('Preflight', 'CRITICAL', f"Scanner binary not found: {self.scanner_binary}")
                pre.warnings.append("Binary missing")
        else:
            self.logger.info(f"Scanner binary: {self.scanner_binary}")

        for p in paths:
            exists = os.path.exists(p)
            pre.paths_exist[p] = exists
            if not exists:
                self.add_error('Preflight', 'ERROR', f"Path does not exist: {p}")
                pre.warnings.append(f"Path missing: {p}")
            else:
                # Readability check (read access to the path)
                readable = os.access(p, os.R_OK)
                pre.paths_readable[p] = readable
                if not readable:
                    self.add_error('Preflight', 'WARNING', f"Path not readable (permissions): {p}")
                    pre.warnings.append(f"Not readable: {p}")
                # Disk space
                st = os.statvfs(p)
                free = st.f_bavail * st.f_frsize
                total = st.f_blocks * st.f_frsize
                pre.disk_space[p] = free
                self.logger.info(f"Path {p}: free={format_bytes(free)} total={format_bytes(total)}")
        self.preflight = pre

    def _command_exists(self, cmd: str) -> bool:
        from shutil import which
        return which(cmd) is not None

    # -------------------------------------------------------------------------
    def phase_discovery(self):
        self.logger.info("=== Phase 2: Discovery ===")
        for p in self.args.paths:
            if not os.path.exists(p):
                continue
            stats = DiscoveryStats(path=p)
            self.logger.info(f"Discovering: {p}")
            start_time = time.time()
            try:
                # Use os.walk with error handling and symlink skip
                for root, dirs, files in os.walk(p, onerror=lambda e: self._walk_error(e, stats), followlinks=False):
                    # Skip symlink dirs
                    dirs[:] = [d for d in dirs if not os.path.islink(os.path.join(root, d))]
                    stats.total_dirs += 1
                    stats.total_files += len(files)
                    for f in files:
                        fp = os.path.join(root, f)
                        if os.path.islink(fp):
                            stats.total_symlinks += 1
                        else:
                            try:
                                stats.total_bytes += os.path.getsize(fp)
                            except OSError:
                                stats.inaccessible_entries += 1
                    # Depth tracking
                    rel = os.path.relpath(root, p)
                    depth = 0 if rel == '.' else rel.count(os.sep) + 1
                    stats.max_depth_reached = max(stats.max_depth_reached, depth)
            except Exception as e:
                self.add_error('Discovery', 'ERROR', f"Error during discovery of {p}: {e}")
            stats.duration_sec = time.time() - start_time
            self.discovery[p] = stats
            self.logger.info(f"Discovery {p}: files={stats.total_files}, dirs={stats.total_dirs}, "
                             f"symlinks={stats.total_symlinks}, bytes={format_bytes(stats.total_bytes)}, "
                             f"depth={stats.max_depth_reached}, duration={stats.duration_sec:.2f}s")

    def _walk_error(self, err, stats: DiscoveryStats):
        stats.inaccessible_entries += 1

    # -------------------------------------------------------------------------
    def phase_execution(self):
        self.logger.info("=== Phase 3: Execution ===")
        # Run scanner_cli once with all paths
        output_csv = self.output_dir / 'results.csv'
        cmd = [self.scanner_binary, str(output_csv)] + self.args.paths
        if self.args.workers > 0:
            cmd += ['--workers', str(self.args.workers)]
        if self.args.include_hidden:
            cmd.append('--include-hidden')
        if self.args.follow_symlinks:
            cmd.append('--follow-symlinks')
        if self.args.timeout:
            cmd += ['--timeout', str(self.args.timeout)]
        self.logger.info(f"Running: {' '.join(cmd)}")
        result = self._run_scan(cmd, output_csv, "single")
        self.scan_results.append(result)
        if result.exit_code != 0:
            self.add_error('Execution', 'ERROR', f"Scanner returned exit code {result.exit_code}", result.stderr[-500:])
        else:
            self.logger.info(f"Scan completed in {result.duration_sec:.2f}s, CSV size={result.csv_size_bytes} bytes")

    def _run_scan(self, cmd: List[str], csv_path: Path, run_id: str) -> ScanRunResult:
        """Execute scanner_cli with resource monitoring."""
        start_ts = utc_now()
        start_time = time.time()
        result = ScanRunResult(run_id=run_id, timestamp_start=start_ts,
                               timestamp_end="", duration_sec=0.0, exit_code=-1,
                               timeout_hit=False, csv_path=str(csv_path))

        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                text=True, bufsize=1)
        monitor = ResourceMonitor(proc.pid, interval=PROGRESS_INTERVAL_SEC)
        monitor.start()
        try:
            stdout, stderr = proc.communicate(timeout=self.args.timeout if self.args.timeout else DEFAULT_TIMEOUT_SCAN_SEC)
            result.timeout_hit = False
            result.exit_code = proc.returncode
            result.stdout = stdout
            result.stderr = stderr
        except subprocess.TimeoutExpired:
            result.timeout_hit = True
            result.exit_code = -1
            proc.kill()
            stdout, stderr = proc.communicate()
            result.stdout = stdout
            result.stderr = stderr
            self.add_error('Execution', 'ERROR', f"Scan timed out after {self.args.timeout or DEFAULT_TIMEOUT_SCAN_SEC}s")
        finally:
            samples = monitor.stop()
            result.metrics = samples
            if samples:
                result.peak_rss_mb = max(s['rss_mb'] for s in samples)
                result.avg_cpu_percent = sum(s['cpu_percent'] for s in samples) / len(samples)
                result.avg_io_read_mb = sum(s['io_read_mb'] for s in samples) / len(samples)
                result.avg_io_write_mb = sum(s['io_write_mb'] for s in samples) / len(samples)

        result.duration_sec = time.time() - start_time
        result.timestamp_end = utc_now()
        if csv_path.exists():
            result.csv_size_bytes = csv_path.stat().st_size
        return result

    # -------------------------------------------------------------------------
    def phase_stress(self):
        self.logger.info("=== Phase 4: Stress ===")
        if self.args.stress_repetitions <= 1:
            self.logger.info("Stress disabled (--stress-repetitions <=1)")
            return
        output_csv_template = self.output_dir / 'stress_results_{run}.csv'
        for i in range(self.args.stress_repetitions):
            self.logger.info(f"Stress run {i+1}/{self.args.stress_repetitions}")
            # Optionally run in parallel (simple: just sequential for safety)
            output_csv = self.output_dir / f'stress_results_{i}.csv'
            cmd = [self.scanner_binary, str(output_csv)] + self.args.paths
            if self.args.workers > 0:
                cmd += ['--workers', str(self.args.workers)]
            result = self._run_scan(cmd, output_csv, f"stress_{i}")
            self.scan_results.append(result)
            self.logger.info(f"Stress run {i+1} finished: exit={result.exit_code}, duration={result.duration_sec:.2f}s")

    # -------------------------------------------------------------------------
    def phase_validation(self):
        self.logger.info("=== Phase 5: Validation ===")
        if not self.scan_results:
            self.add_error('Validation', 'CRITICAL', "No scan results to validate")
            return
        # Use the first scan result (single run)
        primary = self.scan_results[0]
        csv_path = Path(primary.csv_path)
        validation = CSVValidationResult()
        validation.exists = csv_path.exists()
        if not validation.exists:
            validation.warnings.append("CSV file does not exist")
            self.csv_validation = validation
            return
        validation.size_bytes = csv_path.stat().st_size
        if validation.size_bytes == 0:
            validation.warnings.append("CSV is empty")
            self.csv_validation = validation
            return

        # Read CSV with error handling
        try:
            with open(csv_path, 'r', encoding='utf-8') as f:
                # Detect delimiter (should be comma)
                sample = f.readline()
                delimiter = ',' if ',' in sample else ';' if ';' in sample else '\t'
                validation.delimiter = delimiter
                f.seek(0)
                reader = csv.DictReader(f, delimiter=delimiter)
                validation.header = reader.fieldnames or []
                validation.expected_columns = ['path','type','logical_size','allocated_size','hardlink_count','is_symlink']
                validation.missing_columns = [c for c in validation.expected_columns if c not in validation.header]
                rows = list(reader)
                validation.row_count = len(rows)
                validation.read_ok = True
        except UnicodeDecodeError as e:
            validation.read_ok = False
            validation.encoding_issues.append(f"UTF-8 decode error: {e}")
        except csv.Error as e:
            validation.read_ok = False
            validation.encoding_issues.append(f"CSV parse error: {e}")
        except Exception as e:
            validation.read_ok = False
            validation.encoding_issues.append(f"Unexpected error reading CSV: {e}")

        if validation.read_ok:
            # Check for null/empty cells
            import math
            for i, row in enumerate(rows):
                for col in validation.expected_columns:
                    val = row.get(col, '')
                    if val is None or (isinstance(val, str) and val.strip() == ''):
                        validation.null_or_empty_count += 1
                # Type checks
                try:
                    int(row.get('logical_size', '0'))
                    int(row.get('allocated_size', '0'))
                    int(row.get('hardlink_count', '0'))
                except ValueError:
                    validation.invalid_type_count += 1
                # Check is_symlink is boolean-like
                val = row.get('is_symlink', '').lower()
                if val not in ('true','false','0','1'):
                    validation.invalid_type_count += 1
            # Duplicate rows: exact same path+type
            seen = set()
            for row in rows:
                key = (row.get('path',''), row.get('type',''))
                if key in seen:
                    validation.duplicate_rows += 1
                seen.add(key)

            # Cross-check with discovery stats (approximate)
            total_discovered_files = sum(s.total_files for s in self.discovery.values())
            if total_discovered_files > 0:
                # Number of file entries in CSV (type=1)
                file_entries = sum(1 for r in rows if r.get('type') == '1')
                ratio = file_entries / total_discovered_files if total_discovered_files else 0
                if ratio < 0.5 or ratio > 1.5:
                    validation.warnings.append(f"File count mismatch: CSV={file_entries}, discovered={total_discovered_files}")
        self.csv_validation = validation

    # -------------------------------------------------------------------------
    def phase_error_analysis(self):
        self.logger.info("=== Phase 7: Error Analysis ===")
        # Errors already collected in self.errors during phases.
        # Classify by severity
        crit = [e for e in self.errors if e.severity == 'CRITICAL']
        errs = [e for e in self.errors if e.severity == 'ERROR']
        warns = [e for e in self.errors if e.severity == 'WARNING']
        self.logger.info(f"Error counts: CRITICAL={len(crit)}, ERROR={len(errs)}, WARNING={len(warns)}")
        # Analyze stderr from scan for known patterns
        for res in self.scan_results:
            stderr = res.stderr
            if 'Permission denied' in stderr or 'Access denied' in stderr:
                self.add_error('ErrorAnalysis', 'WARNING', "Permission errors detected in scan")
            if 'timeout' in stderr.lower():
                self.add_error('ErrorAnalysis', 'ERROR', "Timeout detected in scan output")
            if 'Exception' in stderr:
                self.add_error('ErrorAnalysis', 'ERROR', "Exception in scanner output")

    # -------------------------------------------------------------------------
    def phase_final_report(self):
        self.logger.info("=== Phase 8: Final Report ===")
        # Determine status
        status = 'PASS'
        if any(e.severity == 'CRITICAL' for e in self.errors):
            status = 'FAIL'
        elif any(e.severity == 'ERROR' for e in self.errors):
            status = 'PASS_WITH_WARNINGS'
        elif any(e.severity == 'WARNING' for e in self.errors):
            status = 'PASS_WITH_WARNINGS'
        # Additional checks
        if self.csv_validation and not self.csv_validation.exists:
            status = 'FAIL'
        if self.scan_results and self.scan_results[0].timeout_hit:
            status = 'FAIL'
        if self.args.require_zero_errors and any(e.severity in ('ERROR','CRITICAL') for e in self.errors):
            status = 'FAIL'

        # Build summary
        summary = {
            'test_start_time': min((r.timestamp_start for r in self.scan_results), default=utc_now()),
            'test_end_time': max((r.timestamp_end for r in self.scan_results), default=utc_now()),
            'paths_scanned': self.args.paths,
            'scanner_binary': self.scanner_binary,
            'total_scans': len(self.scan_results),
            'first_scan_duration_sec': self.scan_results[0].duration_sec if self.scan_results else 0,
            'peak_rss_mb': max((r.peak_rss_mb for r in self.scan_results), default=0),
            'avg_cpu_percent': sum(r.avg_cpu_percent for r in self.scan_results) / len(self.scan_results) if self.scan_results else 0,
            'total_files_discovered': sum(s.total_files for s in self.discovery.values()),
            'total_bytes_discovered': sum(s.total_bytes for s in self.discovery.values()),
            'csv_rows': self.csv_validation.row_count if self.csv_validation else 0,
            'csv_duplicates': self.csv_validation.duplicate_rows if self.csv_validation else 0,
            'csv_null_empty': self.csv_validation.null_or_empty_count if self.csv_validation else 0,
            'csv_validation_passed': self.csv_validation.read_ok if self.csv_validation else False,
            'error_counts': {
                'critical': len([e for e in self.errors if e.severity=='CRITICAL']),
                'error': len([e for e in self.errors if e.severity=='ERROR']),
                'warning': len([e for e in self.errors if e.severity=='WARNING']),
            }
        }

        phases = {
            'preflight': 'OK' if self.preflight and not any(e.severity=='CRITICAL' for e in self.errors if e.phase=='Preflight') else 'ISSUES',
            'discovery': 'OK' if self.discovery else 'ISSUES',
            'execution': 'OK' if self.scan_results and self.scan_results[0].exit_code == 0 else 'FAILED',
            'stress': 'OK' if self.scan_results else 'NOT_RUN',
            'validation': 'OK' if self.csv_validation and self.csv_validation.read_ok else 'FAILED',
            'monitoring': 'OK' if any(r.metrics for r in self.scan_results) else 'NO_DATA',
            'error_analysis': 'DONE',
        }

        self.report = FinalReport(status=status, summary=summary, phases=phases, errors=self.errors)
        self._write_artifacts()

    def _write_artifacts(self):
        out = self.output_dir
        # Write summary JSON
        summary_path = out / 'summary.json'
        with open(summary_path, 'w') as f:
            json.dump({
                'status': self.report.status,
                'summary': self.report.summary,
                'phases': self.report.phases,
                'errors': [asdict(e) for e in self.report.errors],
            }, f, indent=2)
        self.report.artifacts['summary.json'] = str(summary_path)

        # Write errors log
        errors_path = out / 'errors.log'
        with open(errors_path, 'w') as f:
            for e in self.report.errors:
                f.write(f"{e.timestamp} [{e.severity}] {e.phase}: {e.message} {e.details}\n")
        self.report.artifacts['errors.log'] = str(errors_path)

        # Write metrics CSV (combine all samples)
        if self.scan_results:
            metrics_path = out / 'metrics.csv'
            with open(metrics_path, 'w', newline='') as f:
                writer = csv.DictWriter(f, fieldnames=['run_id','timestamp','cpu_percent','rss_mb','io_read_mb','io_write_mb'])
                writer.writeheader()
                for res in self.scan_results:
                    for s in res.metrics:
                        writer.writerow({
                            'run_id': res.run_id,
                            'timestamp': s['timestamp'],
                            'cpu_percent': s['cpu_percent'],
                            'rss_mb': s['rss_mb'],
                            'io_read_mb': s['io_read_mb'],
                            'io_write_mb': s['io_write_mb'],
                        })
            self.report.artifacts['metrics.csv'] = str(metrics_path)

        # Copy primary results CSV
        if self.scan_results and Path(self.scan_results[0].csv_path).exists():
            import shutil
            primary_csv = out / 'results.csv'
            shutil.copy(self.scan_results[0].csv_path, primary_csv)
            self.report.artifacts['results.csv'] = str(primary_csv)

        # Write HTML report
        html_path = out / 'report.html'
        self._generate_html_report(html_path)
        self.report.artifacts['report.html'] = str(html_path)

        # Write test.log already done
        self.report.artifacts['test.log'] = str(self.logger.log_file)

        self.logger.info(f"Artifacts written to: {out}")

    def _generate_html_report(self, path: Path):
        html = f"""<!DOCTYPE html>
<html>
<head><title>E2E Scanner Test Report</title>
<style>
body {{ font-family: sans-serif; margin: 2em; }}
h1 {{ color: #333; }}
.status-PASS {{ color: green; font-weight: bold; }}
.status-PASS_WITH_WARNINGS {{ color: orange; font-weight: bold; }}
.status-FAIL {{ color: red; font-weight: bold; }}
.status-ERROR {{ color: darkred; font-weight: bold; }}
table {{ border-collapse: collapse; width: 100%; margin-bottom: 1em; }}
th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
th {{ background-color: #f2f2f2; }}
.metric {{ font-family: monospace; }}
</style>
</head>
<body>
<h1>Scanner E2E Test Report</h1>
<p>Status: <span class="status-{self.report.status}">{self.report.status}</span></p>
<p>Generated: {utc_now()}</p>
<h2>Summary</h2>
<table>
<tr><th>Metric</th><th>Value</th></tr>
"""
        for k,v in self.report.summary.items():
            html += f"<tr><td>{k}</td><td>{v}</td></tr>\n"
        html += "</table>\n<h2>Phases</h2><table><tr><th>Phase</th><th>Status</th></tr>"
        for k,v in self.report.phases.items():
            html += f"<tr><td>{k}</td><td>{v}</td></tr>\n"
        html += "</table>\n<h2>Errors</h2><ul>"
        for e in self.report.errors:
            html += f"<li>[{e.severity}] {e.phase}: {e.message}</li>"
        html += "</ul>\n</body></html>"
        with open(path, 'w') as f:
            f.write(html)

    # -------------------------------------------------------------------------
    def run(self):
        start_total = time.time()
        self.logger.info("Starting E2E Scanner Test")
        self.logger.info(f"Arguments: {vars(self.args)}")

        self.phase_preflight()
        self.phase_discovery()
        self.phase_execution()
        self.phase_stress()
        self.phase_validation()
        self.phase_error_analysis()
        self.phase_final_report()

        total_duration = time.time() - start_total
        self.logger.info(f"Test finished in {total_duration:.2f}s with status {self.report.status}")
        # Print summary to stdout
        print("\n=== FINAL REPORT ===")
        print(f"Status: {self.report.status}")
        print(f"Total duration: {total_duration:.2f}s")
        print(f"Scans completed: {len(self.scan_results)}")
        print(f"CSV rows: {self.csv_validation.row_count if self.csv_validation else 0}")
        print(f"Artifacts: {', '.join(self.report.artifacts.values())}")
        # Exit code
        if self.report.status == 'PASS':
            sys.exit(0)
        elif self.report.status == 'PASS_WITH_WARNINGS':
            sys.exit(1)
        else:
            sys.exit(2)

# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description='E2E stress and validation test for scanner')
    parser.add_argument('--paths', nargs='+', required=True, help='Directories to scan')
    parser.add_argument('--output-dir', default='./e2e_results', help='Directory for output artifacts')
    parser.add_argument('--scanner-binary', default=None, help='Path to scanner_cli binary (auto-detect if omitted)')
    parser.add_argument('--workers', type=int, default=0, help='Number of worker threads (0=auto)')
    parser.add_argument('--include-hidden', action='store_true', help='Include hidden files')
    parser.add_argument('--follow-symlinks', action='store_true', help='Follow symlinks (dangerous)')
    parser.add_argument('--timeout', type=int, default=DEFAULT_TIMEOUT_SCAN_SEC, help='Timeout per scan in seconds')
    parser.add_argument('--stress-repetitions', type=int, default=DEFAULT_STRESS_REPETITIONS, help='Number of stress repetitions')
    parser.add_argument('--require-zero-errors', action='store_true', help='Fail if any error (not just critical)')
    args = parser.parse_args()

    test = E2ETest(args)
    test.run()

if __name__ == '__main__':
    main()