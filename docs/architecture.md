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
| `src/soc/ti/lm3s6965/`, `src/soc/st/stm32f407/` | Device definitions and UART device registration; STM32 peripheral vectors |
| `src/board/` | Memory regions, board clock policy, pin setup, polling UART, panic output |
| `src/kernel/` | Initialization, uptime, session state, newlib hooks and heap accounting |
| `src/subsystems/vfs/` | Device nodes, descriptors, RAM files/directories, path handling |
| `src/subsystems/shell/` | Input, parsing, dispatch, built-ins, BASIC integration |
| `include/homecore/` | Public C interfaces |
| `cmake/`, `configs/`, `scripts/` | Target selection, linking, configuration, QEMU launcher |
| `tests/` | Host regressions and QEMU command regression |
| `external/` | CMSIS, G2BASIC, and doxygen-awesome-css submodules; pinned ST device headers |

## Build selection

`HOMECORE_BOARD` selects `src/board/<name>/board.cmake`, which sets
`HOMECORE_SOC` and memory regions. The SoC's `soc.cmake` selects `HOMECORE_ARCH`.
The architecture's `arch.cmake` supplies CPU identity and linker sections.
The corresponding CMake subdirectories add sources and include paths.

The `homecore_cpu` interface target applies CPU, Thumb, and software-float flags
to both HomeCore and G2BASIC and to the final link. The toolchain selects GNU
Arm tools. Kconfig generates `include/homecore/autoconf.h`, `.config`, and
`kconfig.cmake` inside the build directory during CMake configuration.

## Startup

1. The vector table supplies the initial main stack pointer and `Reset_Handler`.
2. Shared reset code copies `.data` from flash to RAM and clears `.bss`, then
   calls `main()`. It does not call ST's `SystemInit()` or vendor startup code.
3. `arch_init()` sets VTOR to the linked vector table and configures exception
   priorities.
4. `soc_init()` registers UART device nodes. This happens before board peripheral
   initialization, so registration must not require a functioning console.
5. `board_init()` initializes board hardware. The STM32 port selects HSI and
   configures USART2; the QEMU board relies on the emulated reset configuration.
6. `k_init()` starts the 1 kHz timer, registers `/dev/uptime`, and opens
   `/dev/uart0` three times for stdin, stdout, and stderr.
7. `shell_init()` prepares command handling; `main()` prints the banner and enters
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

SysTick calls `k_tick()`. Uptime reads mask interrupts around the 64-bit counter
copy. Masking interrupts across multiple ticks can lose time. SVC and PendSV
currently panic; stack initialization and switch request helpers are scaffolding.
Hardware FPU context preservation is not implemented.

## I/O, files, and sessions

The usual output path is `printf` → newlib `_write` → VFS descriptor → UART
node operations → peripheral. On STM32 the node uses the board UART functions;
on LM3S the SoC node directly accesses the UART registers. Polling reads block
until a character arrives. `/dev/console` is not currently an alias.

VFS uses fixed metadata/descriptor pools and heap-backed RAM file contents.
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
