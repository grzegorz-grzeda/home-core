# Changelog

All notable changes to HomeCore are recorded here. The format is based on
[Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) as
described in [versioning](docs/versioning.md).

## [0.0.5] - 2026-09-29

### Fixed
- BASIC expressions can no longer overflow the stack: G2Basic 0.1.1 limits
  nesting of parentheses, unary signs, function calls, and `IF ... THEN` per
  line, and deeper lines fail with "expression too deeply nested". Previously
  about 10 levels exhausted the 2 KB stack and silently corrupted the heap.

### Added
- `CONFIG_HOMECORE_SHELL_BASIC_MAX_NESTING` (default 8), passed to G2Basic.

### Changed
- The default main stack grows from 2048 to 3072 bytes, the measured worst
  case at 8 nesting levels (about 2.7 KB) plus a margin.
- `stm32vldiscovery` uses a 2048-byte stack and a nesting limit of 4; its heap
  is about 4.2 KB.

## [0.0.4] - 2026-09-29

### Changed
- The VFS allocates RAM directories, RAM files, and open descriptors from the
  heap on demand and frees them on removal or close. Static RAM drops by about
  6.4 KB with the default configuration; the Kconfig maxima remain as caps.
- `stm32vldiscovery` keeps the default VFS caps, now that unused capacity costs
  no RAM; its heap grows to about 4.7 KB.

### Added
- `ENOMEM` from `vfs_mkdir()` and `vfs_open()` when the heap is exhausted. A
  failed open creates, truncates, and allocates nothing.
- `is_owned` in `vfs_node_t`, managed by the VFS; drivers leave it `false`.

## [0.0.3] - 2026-09-29

### Added
- `stm32vldiscovery` board (STM32F100RB, 8 KB RAM) with a USART1 console and
  24 MHz PLL setup, plus a `stm32vldiscovery-qemu` build that runs in QEMU's
  `stm32vldiscovery` machine. Physical hardware is not yet validated.
- `CONFIG_HOMECORE_BOARD_CLOCK_SETUP` to skip board clock setup on emulators
  that do not model the clock controller.
- `CONFIG_HOMECORE_KERNEL_STDIO_BUFFERED` to make stdin and stdout unbuffered,
  saving about 2 KB of heap.
- Per-board default defconfigs (`configs/<board>_defconfig`).
- Main-stack high-water measurement: `k_stack_stats()` and a
  `Stack: used N of M bytes` line in `mem`.

### Fixed
- The default main stack grows from 1024 to 2048 bytes. Measurements showed
  BASIC using about 0.8 KB before any expression nesting, so moderately nested
  expressions overflowed the old reservation into the heap without detection.

## [0.0.2] - Baseline

Baseline for this changelog, when the versioning policy was adopted. Earlier
changes are recorded only in the git history. This version provides:

- UART shell with file, session, and system commands, and the G2Basic
  interpreter through `basic`.
- VFS with device nodes, RAM files and directories, and per-session working
  directories.
- Newlib integration for console I/O and heap, and a 1 kHz SysTick uptime clock.
- Boards: `lm3s6965evb` (QEMU-tested) and `stm32f4discovery` (cross-compiled;
  hardware validation pending).
