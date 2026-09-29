# Adding a board or SoC

Read [architecture](architecture.md) first. A board using an existing supported
SoC may need only a board directory and preset. A new chip also needs a SoC port;
a new CPU requires architecture support. Use the STM32 and LM3S ports as examples,
while preserving their existing behavior.

## Board integration

Create `src/board/<name>/` with:

- `board.cmake`: set `HOMECORE_SOC` and `HOMECORE_BOARD_LINKER_SCRIPT` using
  `CMAKE_CURRENT_LIST_DIR`.
- `board.yaml`: the board's clocks (`cpu` and every bus a device uses), the
  devices it enables with their properties, and `chosen.console`. Memory comes
  from the SoC description; override `memory` here only if the board differs.
  See [device description](development.md#device-description).
- `CMakeLists.txt`: add board sources and board identity definitions.
- `board.c`: implement the interface in `include/homecore/board/board.h`.
- `board.ld`: additional board sections, or an empty commented fragment.

`board_init()` configures clocks, peripheral clock gates, and GPIO alternate
functions for the enabled devices; the drivers program the devices themselves
in `dt_init()`. `board_cpu_clock_hz()` must return `DT_CPU_CLOCK_HZ`, and
`board.yaml` must state the clock `board_init()` actually produces; a
`_Static_assert` against `DT_CPU_CLOCK_HZ` keeps them in step, as in the STM32
boards. If an
emulator runs the board but does not model its clock controller, put the clock
setup under `CONFIG_HOMECORE_BOARD_CLOCK_SETUP` and provide an emulator
defconfig that disables it, as `stm32vldiscovery` does.
Implement `board_panic()`: print through `board_uart_putc()` only when
`console_ready()`, then `console_flush()` and halt. The `board_uart_*`
functions are provided by `src/drivers/console.c` for the chosen console.

Add a configure/build preset with its own output directory and a board guide
covering flashing, pin wiring, clock/memory choices, limitations, and validation.
Add the board to the API site as well: a page in `docs/doxygen/boards/`, a card
and feature-matrix column in `boards.dox`, and a row on the main page in
`docs/doxygen/groups.dox`. See [board pages](development.md#board-pages).
A board with little RAM can provide `configs/<board>_defconfig`, which CMake
uses by default; check the stack high-water mark from `mem` when sizing it.

## SoC integration

Create `src/soc/<vendor>/<chip>/` with `soc.cmake`, `CMakeLists.txt`, `soc.c`,
`soc_cmsis.h`, `soc.yaml`, and a linker fragment. `soc.yaml` gives the flash
and RAM banks at their real addresses (STM32F407's CCM must not be merged into
the contiguous main SRAM region) and every supported peripheral instance with
`status: disabled`; add a driver and binding for a new peripheral type as
described in [extending](extending.md#device-drivers). Set `HOMECORE_ARCH`, `HOMECORE_SOC_ID`, and
`HOMECORE_SOC_LINKER_SCRIPT`. Make `soc_cmsis.h` expose the chip's CMSIS IRQ/core
configuration and peripheral definitions to the shared architecture sources.
Pin vendor sources and retain their license and provenance.

`main()` calls `soc_init()` before `board_init()`, for chip-level setup that
must precede board configuration. Devices are not registered there: `dt_init()`
instantiates them after `board_init()`. The kernel opens the chosen console
(`DT_CHOSEN_CONSOLE_PATH`) for descriptors 0, 1, and 2; boards keep it at
`/dev/uart0` with the `devname` property.

Provide the chip's peripheral vector entries in `.isr_vector.soc`, immediately
after the 16 `.isr_vector.arch` entries. Preserve reserved slots and IRQ order;
use safe default handlers for unimplemented interrupts. Do not link a second
vendor reset handler/vector table alongside HomeCore's shared startup.

## Architecture and linker integration

Cortex-M3 and M4 wrappers share `src/arch/arm/cortex-m`. A new wrapper needs
`arch.cmake` and `CMakeLists.txt`, selecting the CPU and linker fragment. If the
CPU cannot use the shared implementation, implement the architecture interface
and startup requirements explicitly.

CPU/ABI settings belong in the common build mechanism (`homecore_cpu`), not
scattered source directories. Apply them to all linked code. Keep software
floating point until both exception handling and context switching preserve
any required FPU state.

Preserve startup symbols and their alignment, vector placement and VTOR setup,
flash load addresses for `.data`, and the heap/main-stack reservation. Add
linker checks for any new layout assumptions. See `cmake/linker.ld.in` and the
shared `arch.ld` for the current contract.

## Validation

1. Build the new target in Debug and Release and inspect compiler/linker flags.
2. Inspect vector entries, flash/RAM sections, startup alignment, and heap/stack
   bounds in the ELF/map. Verify the binary's flash programming address.
3. Rebuild both existing targets after shared changes and run the LM3S QEMU
   regression. Run relevant host tests for portable logic changes.
4. On hardware, verify reset entry, banner and interactive console, uptime,
   heap reporting, RAM-file operations, BASIC, and reboot. Confirm uptime against
   elapsed time and check the actual console wiring.
5. Record tested hardware revision and setup; keep untested claims explicit.
   Add new targets to `TARGETS` (and `QEMU_TARGETS` if emulated) in
   `scripts/check_quality.py` and to the build matrix in
   `.github/workflows/build.yml`; for an emulated board, add it to
   `BOARDS` in `tests/qemu_files_test.py`.

A booting shell does not validate scheduling: SVC/PendSV are currently panic
handlers. Peripheral interrupt drivers and thread support require additional
implementation and tests beyond this polling-console bring-up.
