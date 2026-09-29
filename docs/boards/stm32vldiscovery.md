# STM32VLDISCOVERY

The `stm32vldiscovery` target supports ST's STM32VLDISCOVERY board with an
STM32F100RB: Cortex-M3, 128 KB flash at `0x08000000`, and **8 KB RAM** at
`0x20000000`. It has a USART1 console (interrupt-driven receive), the shell and VFS, and a 1 kHz
SysTick. It runs in QEMU's `stm32vldiscovery` machine, which makes it the STM32
code path covered by CI. Physical hardware has not been validated.

## Build configurations

| Preset | Configuration | Clock setup | Use |
| --- | --- | --- | --- |
| `stm32vldiscovery` | `configs/stm32vldiscovery_defconfig` | PLL to 24 MHz | Physical board |
| `stm32vldiscovery-qemu` | same defconfig + overlay `src/board/stm32vldiscovery/qemu.yaml` | Skipped | QEMU only |

```bash
cmake --preset stm32vldiscovery-qemu -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset stm32vldiscovery-qemu
bash scripts/run-qemu-stm32vldiscovery.sh
python3 tests/qemu_files_test.py --board stm32vldiscovery
```

QEMU maps its first serial port to USART1. Exit the interactive session with
Ctrl-A, then X.

## Clock

With `clock_setup` true (the default in `board.yaml`), `board_init()` runs the PLL
from HSI/2 × 6 = 24 MHz, the STM32F100 maximum, with undivided AHB, APB1, and
APB2 clocks. The value line has no flash wait states to configure. Each step
waits with a bounded iteration budget; a failure halts silently before the
console starts, so diagnose it with a debugger.

QEMU does not model the clock controller (RCC) or GPIO: reads return 0 and
writes are ignored. Its CPU and SysTick clock is fixed at 24 MHz. The QEMU
overlay (`qemu.yaml`, `clock_setup: false`) therefore skips clock setup, and the
board reports 24 MHz in both configurations. The GPIO and USART setup runs in both; QEMU ignores the GPIO
writes. Do not flash the QEMU build to a physical board: it would run at the
8 MHz reset clock while assuming 24 MHz.

## Memory

The 8 KB of RAM holds about 1.9 KB of static data, a 2 KB main stack, and a
4.2 KB heap. The board defconfigs change four defaults:

| Setting | Default | This board |
| --- | --- | --- |
| `CONFIG_HOMECORE_KERNEL_MAIN_STACK_SIZE` | 3072 | 2048 |
| `CONFIG_HOMECORE_SHELL_BASIC_MAX_NESTING` | 8 | 4 |
| `CONFIG_HOMECORE_VFS_MAX_FILE_SIZE` | 4096 | 1024 |
| `CONFIG_HOMECORE_KERNEL_STDIO_BUFFERED` | y | n (saves about 2 KB of heap) |

VFS entries and descriptors are allocated only while they exist, so the
default caps of 16 files, 16 directories, and 16 open files are kept. In QEMU,
about 450 bytes of heap are used after boot, and 16 directories plus 16 empty
files add about 2 KB. File contents, BASIC programs, and variables share the
remaining heap; when it runs out, creating an entry fails with "Not enough
space" (`ENOMEM`), which differs from "No space left on device" at a cap.

BASIC lines may nest parentheses, unary signs, function calls, and
`IF ... THEN` up to 4 levels; deeper lines fail with "expression too deeply
nested". Measured stack peaks in QEMU are about 0.9 KB for file commands and
1.7 KB for BASIC at the nesting limit (four nested function calls, the most
expensive case), leaving about 380 bytes of margin. `mem` reports the heap and
the stack high-water mark.

## Physical board (not validated)

Connect a 3.3 V USB-to-UART adapter at 115200 baud, 8N1, no flow control:

| Board pin | Adapter pin |
| --- | --- |
| PA9 (USART1 TX) | RX |
| PA10 (USART1 RX) | TX |
| GND | GND |

The board's ST-LINK/V1 has no virtual COM port. OpenOCD provides
`board/stm32vldiscovery.cfg` for it, and on Linux the ST-LINK/V1 needs the
`usb-storage` quirk `483:3744:i`. These flashing steps have not been checked
with this port:

```bash
openocd -f board/stm32vldiscovery.cfg \
  -c "program build/stm32vldiscovery/homecore verify reset exit"
```

Validation on hardware should confirm the banner, `uptime` against elapsed
time (which checks the 24 MHz PLL), `mem`, file commands, `basic`, and
`reboot`, as described in [porting](../porting.md#validation).

See [development](../development.md) for prerequisites and validation commands.
