# STM32F4DISCOVERY

The `stm32f4discovery` target supports the STM32F407VGT6 board with a polling
USART2 console, shell/VFS, and 1 kHz SysTick. It uses the 16 MHz internal HSI
clock, software floating point, 1 MB flash, and 128 KB main SRAM. The separate
64 KB CCM RAM is unused. USB, audio, sensors, and hardware FPU context switching
are not implemented. SVC/PendSV remain the existing panic handlers.

Build with an Arm GNU bare-metal toolchain, CMake, Ninja, and Python:

```bash
git submodule update --init --recursive
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
cmake --preset stm32f4discovery -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset stm32f4discovery
```

The ELF is `build/stm32f4discovery/homecore`; the raw flash image is
`build/stm32f4discovery/homecore.bin`. With OpenOCD installed and the on-board
ST-LINK connected (CN3 jumpers fitted), program and reset with:

```bash
openocd -f board/stm32f4discovery.cfg \
  -c "program build/stm32f4discovery/homecore verify reset exit"
```

Connect a **3.3 V USB-to-UART adapter** and use **115200 baud, 8N1, no flow control**:

| Board pin | Adapter pin |
| --- | --- |
| PA2 (USART2 TX) | RX |
| PA3 (USART2 RX) | TX |
| GND | GND |

Power the board through its ST-LINK USB connector; leave the adapter power pin
disconnected. Keep BOOT0 low for flash boot. The console appears as `/dev/uart0`.
The original STM32F4DISCOVERY has ST-LINK/V2 and needs an external UART adapter.
The STM32F407G-DISC1 has ST-LINK/V2-A with VCP support, but its UART pins are
not connected to the target by default. To use that VCP, connect PA2 (P1 pin 14)
to U2 pin 13 (ST-LINK RX), and PA3 (P1 pin 13) to U2 pin 12 (ST-LINK TX).
These are added wires to the ST-LINK MCU pins, not the CN3 SWD jumpers. See [ST's board manual (UM1472)](https://www.st.com/resource/en/user_manual/um1472-stm32f4-discovery-stmicroelectronics.pdf).
Configure the terminal to display LF as a new line; press Enter to submit commands.

After reset, check the HomeCore banner and prompt, then run `help`, `mem`,
`uptime`, `cat /dev/uptime`, `mkdir /tmp`, `touch /tmp/test`, `ls /tmp`, and
`reboot`. Uptime accuracy follows HSI oscillator tolerance. Polling RX can lose
characters during lengthy commands; send input interactively rather than
pasting large programs at full serial speed.

This port has been cross-compiled and its linked layout checked. The existing
LM3S6965EVB QEMU command regression passes; STM32 hardware validation is still
required. ST's pinned CMSIS device headers and license are in `external/stm32f4`.

See [development](../development.md) for prerequisites and validation commands.
