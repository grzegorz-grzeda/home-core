# Architecture

HomeCore currently executes a shell and BASIC in a single foreground flow with
SysTick interrupts. There is no running scheduler or user/kernel privilege
boundary. Public architecture declarations include future-facing APIs; consult
the implementation before relying on them.

## Source layout and ownership

| Path | Responsibility |
| --- | --- |
| `src/main.c` | Initialization order and foreground shell entry |
| `src/arch/arm/cortex-m/` | Shared reset startup, core exceptions, IRQ/timer helpers, stack frame setup, linker sections |
| `src/arch/arm/cortex-m3/`, `cortex-m4/` | Architecture selection and CPU identity |
| `src/soc/ti/lm3s6965/`, `src/soc/st/stm32f407/`, `src/soc/st/stm32f100/` | Device definitions, peripheral vectors, and `soc.yaml` (memory and peripheral instances) |
| `src/board/` | `board.yaml` (clocks, enabled devices, console), clock policy, pin and clock-gate setup, panic output |
| `src/drivers/` | Device drivers and their bindings (`<compatible>.yaml`); `console.c` implements `board_uart_*` on the chosen console |
| `src/kernel/` | Initialization, uptime, session state, newlib hooks and heap accounting |
| `src/subsystems/vfs/` | Device nodes, descriptors, RAM files/directories, path handling |
| `src/subsystems/shell/` | Input, parsing, dispatch, built-ins, BASIC integration |
| `include/homecore/` | Public C interfaces |
| `cmake/`, `configs/`, `scripts/` | Target selection, device description, linking, configuration, QEMU launcher |
| `tests/` | Host regressions and QEMU command regression |
| `external/` | CMSIS, G2BASIC, and doxygen-awesome-css submodules; pinned ST device headers |

## Build selection

`HOMECORE_BOARD` selects `src/board/<name>/board.cmake`, which sets
`HOMECORE_SOC`. The SoC's `soc.cmake` selects `HOMECORE_ARCH`. The
architecture's `arch.cmake` supplies CPU identity and linker sections. The
corresponding CMake subdirectories add sources and include paths.

Hardware is described in data, not code: `soc.yaml` lists the chip's memory
and peripheral instances (all disabled), `board.yaml` enables devices and sets
their properties and the clocks, and optional overlays
(`HOMECORE_DT_OVERLAYS`) apply last. At configure time
`scripts/devicetree_generate.py` merges them, validates each enabled device
against its driver's binding, and generates `homecore/devicetree.h`
(constants such as `DT_CPU_CLOCK_HZ` and `DT_CHOSEN_CONSOLE_PATH`),
`devicetree.c` (`static const` driver configurations, driver instances,
`dt_console`, and `dt_init()`), the linker `MEMORY` block, and the list of
driver sources. Only drivers of enabled devices are compiled. Nothing is
parsed at run time. See [development](development.md#device-description).

The `homecore_cpu` interface target applies CPU, Thumb, and software-float flags
to both HomeCore and G2BASIC and to the final link. The toolchain selects GNU
Arm tools. Kconfig generates `include/homecore/autoconf.h`, `.config`, and
`kconfig.cmake` inside the build directory during CMake configuration.

## Startup

1. The vector table supplies the initial main stack pointer and `Reset_Handler`.
2. Shared reset code fills the unused main stack with `K_STACK_FILL_WORD` for
   high-water measurement, copies `.data` from flash to RAM, clears `.bss`, then
   calls `main()`. It does not call ST's `SystemInit()` or vendor startup code.
3. `arch_init()` sets VTOR to the linked vector table and configures exception
   priorities.
4. `soc_init()` performs chip-level setup; the current SoCs need none.
5. `board_init()` configures clocks, peripheral clock gates, and pins.
   STM32F4DISCOVERY selects HSI and prepares USART2. STM32VLDISCOVERY runs the
   PLL at 24 MHz, skipped in its QEMU build, and prepares USART1. LM3S6965EVB
   relies on QEMU's emulated reset configuration.
6. `dt_init()` initializes every enabled device in description order; each
   driver programs its hardware and registers its `/dev` node. Until then the
   console is not ready, and `board_panic()` halts silently.
7. `k_init()` starts the 1 kHz timer, registers `/dev/uptime`, and opens the
   chosen console (`DT_CHOSEN_CONSOLE_PATH`, `/dev/uart0` on every board) three
   times for stdin, stdout, and stderr.
8. `shell_init()` prepares command handling; `main()` prints the banner and enters
   `shell_run()`.

VFS global state starts zero-initialized; device registration is part of startup.
Do not add a late `vfs_init()` call that clears already registered devices.

## Memory and exceptions

The generated linker script combines board memory regions with architecture,
SoC, and board sections. The 16 core vectors precede SoC peripheral vectors.
Code and read-only data live in flash; `.data` executes from RAM with a flash
load image; `.bss` is zeroed RAM.

The linker exports `_sidata`, `_sdata`, `_edata`, `_sbss`, and `_ebss` for startup.
`_estack` is the top of main RAM. `_heap_start` follows static RAM;
`_heap_end` leaves `CONFIG_HOMECORE_KERNEL_MAIN_STACK_SIZE` bytes reserved at
RAM's top. `_sbrk()` provides the heap backing for newlib allocation. There is
no stack guard or memory protection enforcing the reservation at runtime.
`k_stack_stats()` reports the deepest stack use since reset (shown by `mem`), so
overflow can be noticed in testing but is not prevented. With
`CONFIG_HOMECORE_KERNEL_STDIO_BUFFERED` disabled, `k_init()` makes stdin and
stdout unbuffered, so newlib never allocates their 1 KB buffers.

SysTick calls `k_tick()`. Uptime reads mask interrupts around the 64-bit counter
copy. Masking interrupts across multiple ticks can lose time. SVC and PendSV
currently panic; stack initialization and switch request helpers are scaffolding.
Hardware FPU context preservation is not implemented.

## I/O, files, and sessions

The usual output path is `printf` → newlib `_write` → VFS descriptor → serial
driver node (`st,stm32-usart` or `ti,stellaris-uart`) → peripheral. The board
interface's `board_uart_*` functions, used by panic paths, go through
`dt_console` to the same driver. Polling reads block until a character
arrives. `/dev/console` is not currently an alias.

The VFS allocates RAM directories, RAM files, and open descriptors from the heap
on demand and frees them on removal or close, so unused capacity costs no RAM.
Kconfig maxima cap their numbers. Only a table of descriptor pointers is static.
Files disappear on reset. Snapshot devices such as `/dev/uptime` capture content
at open; each descriptor has its own position and snapshot. A live UART does
not provide file EOF.

A session holds a user pointer, working directory, and last command status.
The default user is root. Shell commands receive context explicitly; libc path
operations use the current session. Low-level VFS relative paths remain rooted
at `/`; session-aware callers resolve against their working directory first.
The current-session pointer is global and must become task-local before concurrent
sessions can run. Identity is metadata, without authentication or access checks.

The shell embeds G2BASIC through callbacks and registers `millis()` against
kernel uptime. Interpreter integration and libc hooks run in the same address
space as the rest of the firmware; these are not SVC-mediated system calls.
