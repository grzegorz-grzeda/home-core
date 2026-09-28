# LM3S6965EVB in QEMU

This target is the development and regression platform: Cortex-M3, 256 KB flash
at `0x00000000`, and 64 KB RAM at `0x20000000`. It is validated in QEMU;
physical Stellaris board bring-up is not covered.

Build using the [development instructions](../development.md), then run:

```bash
bash scripts/run-qemu-lm3s6965evb.sh
```

The script boots `build/lm3s6965evb/homecore` with QEMU's `lm3s6965evb` machine
and `-nographic`. The first serial console serves the shell. Exit with Ctrl-A,
then X. For regression testing, run `python3 tests/qemu_files_test.py`.

The SoC registers `/dev/uart0`, `/dev/uart1`, and `/dev/uart2`; stdin/stdout/stderr
use UART0. The other nodes are not additional interactive shell sessions.
Board initialization relies on QEMU's emulated reset state rather than configuring
real GPIO, UART baud rates, or a physical oscillator.

`board_cpu_clock_hz()` returns 12.5 MHz for QEMU's Stellaris reset divider
configuration; see the [QEMU model](https://github.com/qemu/qemu/blob/master/hw/arm/stellaris.c).
Uptime follows guest virtual time and stops while emulation is paused. Change
the clock reporting if introducing clock setup or physical hardware support.

Try `help`, `uptime`, `cat /dev/uptime`, `mem`, file commands, and `reboot`.
Reboot discards RAM files and resets the session. QEMU success verifies this
emulated target, not STM32 device register programming or UART wiring.
