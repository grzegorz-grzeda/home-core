# Adding a board or SoC

Read [architecture](architecture.md) first. A board using an existing supported
SoC may need only a board directory and preset. A new chip also needs a SoC port;
a new CPU requires architecture support. Use the STM32 and LM3S ports as examples,
while preserving their existing behavior.

## Board integration

Create `src/board/<name>/` with:

- `board.cmake`: set `HOMECORE_SOC`, `HOMECORE_BOARD_MEMORY`, and
  `HOMECORE_BOARD_LINKER_SCRIPT` using `CMAKE_CURRENT_LIST_DIR`.
- `CMakeLists.txt`: add board sources and board identity definitions.
- `board.c`: implement the interface in `include/homecore/board/board.h`.
- `board.ld`: additional board sections, or an empty commented fragment.

`board_init()` configures clocks, GPIO alternate functions, and the console.
`board_cpu_clock_hz()` must return the actual core clock used by SysTick.
Implement polling UART read/write/availability and a panic path that can halt
safely before the console is ready. Define memory banks by their real addresses;
STM32F407's CCM must not be merged into the contiguous main SRAM region.

Add a configure/build preset with its own output directory and a board guide
covering flashing, pin wiring, clock/memory choices, limitations, and validation.

## SoC integration

Create `src/soc/<vendor>/<chip>/` with `soc.cmake`, `CMakeLists.txt`, `soc.c`,
`soc_cmsis.h`, and a linker fragment. Set `HOMECORE_ARCH`, `HOMECORE_SOC_ID`, and
`HOMECORE_SOC_LINKER_SCRIPT`. Make `soc_cmsis.h` expose the chip's CMSIS IRQ/core
configuration and peripheral definitions to the shared architecture sources.
Pin vendor sources and retain their license and provenance.

`main()` calls `soc_init()` before `board_init()`. Register devices without
accessing uninitialized board hardware. The current kernel expects `/dev/uart0`
and opens it for descriptors 0, 1, and 2. Provide VFS operations as well as board
UART helpers; board-only output is insufficient for libc and the shell.

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
   Add the target to CI when extending build coverage, rather than implying the
   current LM3S-only workflow already checks it.

A booting shell does not validate scheduling: SVC/PendSV are currently panic
handlers. Peripheral interrupt drivers and thread support require additional
implementation and tests beyond this polling-console bring-up.
