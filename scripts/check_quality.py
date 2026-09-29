#!/usr/bin/env python3
"""Run HomeCore's automated quality checks; never rewrite source files.

Exit 0 means all selected automated checks passed, not semantic compliance.
Run from any directory. Build products stay under build/quality/.
"""

import argparse
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FORMAT_VERSION = "19.1.7"
CHECKS = ("format", "headers", "build", "host", "qemu")
# Build targets: name -> (board, device-description overlay relative to the
# root, or None). Each board uses its default defconfig, configs/<board>_defconfig
# or configs/homecore_defconfig.
TARGETS = {
    "lm3s6965evb": ("lm3s6965evb", None),
    "stm32f4discovery": ("stm32f4discovery", None),
    "stm32vldiscovery": ("stm32vldiscovery", None),
    "stm32vldiscovery-qemu": ("stm32vldiscovery", "src/board/stm32vldiscovery/qemu.yaml"),
}
# QEMU regressions: target -> board name passed to tests/qemu_files_test.py.
QEMU_TARGETS = {"lm3s6965evb": "lm3s6965evb", "stm32vldiscovery-qemu": "stm32vldiscovery"}
CONFIGURATIONS = ("Debug", "Release")
# Vendor-generated SVD/CMSIS definitions live outside external/ in the legacy port.
# Files kept in their upstream layout: a vendored header and FatFs's
# configuration file, a modified copy of upstream ffconf.h.
VENDOR_HEADERS = {Path("src/soc/ti/lm3s6965/soc_cmsis.h"), Path("src/subsystems/fs/fat/ffconf.h")}


def source_files(root):
    """Include new/untracked first-party files; exclude external/generated code."""
    return sorted(
        path
        for directory in ("src", "include", "tests")
        for path in (root / directory).rglob("*")
        if path.is_file()
        and not path.is_symlink()
        and path.suffix in (".c", ".h")
        and path.relative_to(root) not in VENDOR_HEADERS
    )


def execute(argv, cwd, timeout):
    """Return (passed, output); kill the entire child group on POSIX timeouts."""
    try:
        process = subprocess.Popen(
            [str(arg) for arg in argv],
            cwd=cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            errors="replace",
            start_new_session=(os.name == "posix"),
        )
    except OSError as error:
        return False, str(error)
    try:
        output, _ = process.communicate(timeout=timeout)
    except (subprocess.TimeoutExpired, KeyboardInterrupt) as error:
        if os.name == "posix":
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
        else:
            process.kill()
        output, _ = process.communicate()
        if isinstance(error, KeyboardInterrupt):
            raise
        return False, f"Timed out after {timeout:g}s\n{output}"
    if process.returncode:
        return False, f"Exit status {process.returncode}\n{output}"
    return True, output


def default_formatter():
    for directory in (Path(sys.executable).parent, ROOT / ".venv/bin"):
        candidate = directory / "clang-format"
        if candidate.is_file():
            return str(candidate)
    return shutil.which("clang-format") or "clang-format"


class QualityRunner:
    def __init__(self, args, root=ROOT):
        self.args = args
        self.root = root
        self.results = []
        self.configured = {}
        self.built = {}

    def record(self, label, passed, output=""):
        self.results.append((label, passed))
        print(f"{'PASS' if passed else 'FAIL'}: {label}", flush=True)
        if output and (not passed or self.args.verbose):
            lines = output.rstrip().splitlines()
            # Keep default failures readable; verbose retains the complete diagnostics.
            shown = lines if self.args.verbose else lines[:20]
            print("\n".join(shown), flush=True)
            if len(shown) < len(lines):
                print("... additional diagnostics omitted; rerun with --verbose", flush=True)
        return passed

    def command(self, label, argv, timeout=None):
        passed, output = execute(argv, self.root, timeout or self.args.timeout)
        return self.record(label, passed, output)

    def formatting(self):
        passed, output = execute(
            [self.args.clang_format, "--version"], self.root, self.args.timeout
        )
        version = re.search(r"\bversion\s+(\d+\.\d+\.\d+)\b", output)
        if not passed or not version or version.group(1) != FORMAT_VERSION:
            self.record(
                "formatter prerequisite", False,
                f"Require clang-format {FORMAT_VERSION}. Install requirements-quality.txt\n{output}",
            )
            return
        files = source_files(self.root)
        if not files:
            self.record("format file discovery", False, "No first-party C files found")
            return
        for path in files:
            self.command(
                f"format {path.relative_to(self.root)}",
                [self.args.clang_format, "--dry-run", "--Werror", path],
            )

    def build_dir(self, target, configuration):
        return self.root / "build/quality" / target / configuration.lower()

    def configure(self, target, configuration):
        key = (target, configuration)
        if key not in self.configured:
            board, overlay = TARGETS[target]
            # Always pass the defconfig and overlays, so a reused build
            # directory cannot keep stale cached values.
            defconfig = self.root / f"configs/{board}_defconfig"
            if not defconfig.exists():
                defconfig = self.root / "configs/homecore_defconfig"
            overlays = str(self.root / overlay) if overlay else ""
            self.configured[key] = self.command(
                f"configure {target} {configuration}",
                [
                    "cmake", "-S", self.root, "-B", self.build_dir(*key), "-G", "Ninja",
                    f"-DCMAKE_TOOLCHAIN_FILE={self.root / 'cmake/toolchains/arm-none-eabi.cmake'}",
                    f"-DHOMECORE_BOARD={board}", f"-DCMAKE_BUILD_TYPE={configuration}",
                    f"-DPython3_EXECUTABLE={self.args.python}",
                    f"-DHOMECORE_DEFCONFIG={defconfig}",
                    f"-DHOMECORE_DT_OVERLAYS={overlays}",
                    "-DCMAKE_COMPILE_WARNING_AS_ERROR=ON",
                ],
            )
        return self.configured[key]

    def build(self, target, configuration):
        key = (target, configuration)
        if key not in self.built:
            if not self.configure(*key):
                self.built[key] = False
                return False
            self.built[key] = self.command(
                f"build {target} {configuration} (warnings are errors)",
                ["cmake", "--build", self.build_dir(*key), "--clean-first"],
            )
        return self.built[key]

    def firmware(self):
        for target in TARGETS:
            for configuration in CONFIGURATIONS:
                self.build(target, configuration)

    def host_flags(self):
        return [
            self.args.cc, "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-Iinclude",
            f"-I{self.build_dir('lm3s6965evb', 'Debug') / 'include'}",
        ]

    def headers(self):
        if not self.configure("lm3s6965evb", "Debug"):
            return
        with tempfile.TemporaryDirectory(prefix="homecore-headers-") as temp:
            unit = Path(temp) / "header.c"
            for header in sorted((self.root / "include/homecore").rglob("*.h")):
                name = header.relative_to(self.root / "include").as_posix()
                unit.write_text(f'#include "{name}"\n', encoding="utf-8")
                self.command(f"self-contained header {name}",
                             self.host_flags() + ["-fsyntax-only", unit])

    def host_tests(self):
        self.command("quality runner self-tests",
                     [self.args.python, "-B", "-m", "unittest", "discover", "-s", "tests",
                      "-p", "quality_runner_test.py"])
        self.command("device description generator tests",
                     [self.args.python, "-B", "-m", "unittest", "discover", "-s", "tests",
                      "-p", "devicetree_generate_test.py"])
        if not self.configure("lm3s6965evb", "Debug"):
            return
        tests = [
            ("uptime", ["tests/uptime_test.c", "src/subsystems/vfs/vfs.c"], []),
            ("directories", ["tests/vfs_directories_test.c", "src/subsystems/vfs/vfs.c"], []),
            ("ram-files", ["tests/vfs_ram_files_test.c", "src/subsystems/vfs/vfs.c"], []),
            ("allocation-failures",
             ["tests/vfs_alloc_failure_test.c", "src/subsystems/vfs/vfs.c"],
             ["-Wl,--wrap=calloc"]),
            ("fat", [
                "tests/fat_test.c", "src/subsystems/vfs/vfs.c", "src/subsystems/fs/fat/fat.c",
                "src/subsystems/fs/fat/diskio.c", "src/drivers/block/ramdisk.c",
                "external/fatfs/ff.c", "external/fatfs/ffsystem.c", "external/fatfs/ffunicode.c",
            ], ["-Isrc/subsystems/fs/fat", "-Iexternal/fatfs", "-Isrc/drivers/block"]),
            ("littlefs", [
                "tests/littlefs_test.c", "src/subsystems/vfs/vfs.c",
                "src/subsystems/fs/littlefs/littlefs.c", "src/drivers/block/ramdisk.c",
                "external/littlefs/lfs.c", "external/littlefs/lfs_util.c",
            ], ["-Isrc/subsystems/fs/littlefs", "-Iexternal/littlefs", "-Isrc/drivers/block",
                "-DLFS_NAME_MAX=127", "-DLFS_NO_DEBUG", "-DLFS_NO_WARN", "-DLFS_NO_ERROR"]),
            ("sessions", [
                "tests/session_test.c", "src/kernel/session.c", "src/subsystems/vfs/vfs.c",
                "src/subsystems/shell/shell.c", "src/subsystems/shell/builtin/builtin_files.c",
                "src/subsystems/shell/builtin/builtin_session.c",
                "src/subsystems/shell/builtin/builtin_system.c",
            ], ["-Isrc/subsystems/shell/builtin"]),
            ("uart-lm3s", ["tests/uart_contract_test.c"],
             ["-Itests/fakes/stellaris", "-Isrc/drivers/serial"]),
            ("uart-stm32", ["tests/uart_contract_test.c"],
             ["-DTEST_STM32", "-Itests/fakes/stm32", "-Isrc/drivers/serial"]),
            ("gpio-stm32f4", ["tests/gpio_contract_test.c"],
             ["-Itests/fakes/stm32f4", "-Isrc/drivers/gpio"]),
            ("gpio-stm32f1", ["tests/gpio_contract_test.c"],
             ["-DTEST_STM32F1", "-Itests/fakes/stm32f1", "-Isrc/drivers/gpio"]),
            ("leds", ["tests/led_test.c", "src/drivers/led/led.c", "src/drivers/led/gpio_led.c",
                      "src/drivers/led/console_led.c", "src/subsystems/vfs/vfs.c"],
             ["-Isrc/drivers/led"]),
            # Documented host-only exception: newlib uses mallinfo; glibc deprecates it.
            ("console-io", ["tests/console_io_test.c"], [
                "-Wno-deprecated-declarations", "-ffunction-sections", "-fdata-sections",
                "-Wl,--gc-sections",
            ]),
        ]
        with tempfile.TemporaryDirectory(prefix="homecore-quality-") as temp:
            for name, sources, flags in tests:
                binary = Path(temp) / name
                if self.command(f"compile host {name}",
                                self.host_flags() + flags + sources + ["-o", binary]):
                    self.command(f"run host {name}", [binary], timeout=10)

    def qemu(self):
        for target, board in QEMU_TARGETS.items():
            for configuration in CONFIGURATIONS:
                if self.build(target, configuration):
                    self.command(
                        f"QEMU regression {target} {configuration}",
                        [self.args.python, "tests/qemu_files_test.py", "--board", board,
                         "--firmware", self.build_dir(target, configuration) / "homecore"],
                    )

    def run(self):
        methods = {"format": self.formatting, "headers": self.headers,
                   "build": self.firmware, "host": self.host_tests, "qemu": self.qemu}
        for check in CHECKS:
            if check in self.args.checks:
                methods[check]()
        failures = sum(not passed for _, passed in self.results)
        print(f"\nAutomated checks: {len(self.results) - failures} passed, {failures} failed.")
        omitted = set(CHECKS) - set(self.args.checks)
        if omitted:
            print("Partial run; not requested: " + ", ".join(sorted(omitted)))
        print("Semantic coding-standard review and physical-board validation remain separate.")
        return 1 if failures else 0


def positive_seconds(value):
    seconds = float(value)
    if not 0 < seconds < float("inf"):
        raise argparse.ArgumentTypeError("timeout must be finite and greater than zero")
    return seconds


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--checks", nargs="+", choices=CHECKS, default=list(CHECKS),
                        help="select checks (default: all); dependencies still run")
    parser.add_argument("--clang-format", default=default_formatter(),
                        help=f"formatter executable; requires {FORMAT_VERSION}")
    python = ROOT / ".venv/bin/python"
    parser.add_argument("--python", default=str(python) if python.exists() else sys.executable,
                        help="Python interpreter with kconfiglib installed")
    parser.add_argument("--cc", default="cc", help="host C compiler executable")
    parser.add_argument("--timeout", type=positive_seconds, default=120,
                        help="per-command timeout in seconds (host test execution: 10s)")
    parser.add_argument("--verbose", action="store_true", help="show complete command output")
    args = parser.parse_args()
    try:
        return QualityRunner(args).run()
    except KeyboardInterrupt:
        print("\nInterrupted: automated review incomplete.", file=sys.stderr)
        return 130
    except OSError as error:
        print(f"\nQuality check could not complete: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
