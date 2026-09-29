# Changelog

All notable changes to HomeCore are recorded here. The format is based on
[Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) as
described in [versioning](docs/versioning.md).

## [0.1.0] - 2026-09-29

### Added
- Interrupt-driven UART receive: each serial device's interrupt fills a
  64-byte ring, and reads sleep with WFI until a byte arrives. Bytes beyond a
  full ring are dropped and counted. On STM32 this removes the one-byte receive
  window that lost pasted input (not yet checked on hardware).
- Peripheral interrupt dispatch through the device description: every vector
  enters `arch_irq_entry()`, and the generated `dt_irq_dispatch()` calls the
  owning driver's interrupt handler or panics with "Unexpected interrupt".
- A peripheral vector table for LM3S6965, which had none.
- `clock_setup` in board descriptions and overlays (`DT_CLOCK_SETUP`), and the
  overlay `src/board/stm32vldiscovery/qemu.yaml`.

### Removed
- **Breaking:** `CONFIG_HOMECORE_BOARD_CLOCK_SETUP` and
  `configs/stm32vldiscovery_qemu_defconfig`. The `stm32vldiscovery-qemu` preset
  now uses the board's defconfig with the `qemu.yaml` overlay. Reconfigure an
  existing `build/stm32vldiscovery-qemu` with `cmake --preset
  stm32vldiscovery-qemu --fresh`, because it caches the removed defconfig.
- Weak per-peripheral `*_IRQHandler` names in the STM32 vector tables; drivers
  receive interrupts through the device description instead.

## [0.0.7] - 2026-09-29

### Changed
- Target selection is part of the device description: `board.yaml` names the
  SoC (`soc:`) and `soc.yaml` names the architecture (`arch:`). The per-board
  `board.cmake` and per-SoC `soc.cmake` files are removed, `soc.ld` and
  `board.ld` are optional, and the unused `HOMECORE_SOC_ID` is gone.
- A layer that sets a key it does not own, such as `arch` in a board, is
  rejected at configure time.

### Fixed
- The `BOARD` macro yields the board name from `board.yaml` (`DT_BOARD_NAME`)
  instead of the literal `"HOMECORE_BOARD_NAME"`. The per-board
  `HOMECORE_BOARD_NAME` and `HOMECORE_BOARD` compile definitions are removed.

## [0.0.6] - 2026-09-29

### Added
- Compile-time device description: `soc.yaml` and `board.yaml` per target,
  merged with optional overlays (`HOMECORE_DT_OVERLAYS`) and validated against
  driver bindings at configure time. It generates `homecore/devicetree.h`
  (`DT_CPU_CLOCK_HZ`, `DT_CHOSEN_CONSOLE_PATH`), driver instances with
  `dt_init()`, and the linker memory map. Only drivers of enabled devices are
  compiled. Requires PyYAML (now in `requirements.txt`).
- Serial drivers `st,stm32-usart` (STM32F1 and STM32F4) and
  `ti,stellaris-uart` (LM3S), and `homecore/drivers/console.h`.

### Changed
- UARTs are instantiated from the description by `dt_init()`, called after
  `board_init()`. The two STM32 boards no longer contain USART code, and the
  `board_uart_*` functions are implemented once on the chosen console.
- Device names are unchanged: `/dev/uart0` on every board, plus `/dev/uart1`
  and `/dev/uart2` on LM3S6965EVB.
- `board_panic()` prints only once the console is initialized; an earlier panic
  on LM3S6965EVB now halts silently.

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
