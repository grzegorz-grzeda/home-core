<p align="center">
  <img src="docs/assets/homecore-logo-wordmark.svg" alt="HomeCore" width="360">
</p>

**HomeCore is a tiny personal computer for STM32 microcontrollers**, in the
spirit of the home computers of the 1980s: switch it on and you get a shell,
files, and BASIC. It runs on real boards and, with no hardware at all, in the
QEMU emulator.

```text
HomeCore OS
Version: 0.1.3
root:/$ cd /ram
root:/ram$ write notes.txt buy solder
root:/ram$ cat notes.txt
buy solder
root:/ram$ write /dev/led0 on
[led0] on
root:/ram$ basic
G2BASIC Interpreter with line numbers. Ctrl-C/Ctrl-D/Ctrl-Z to exit.

> 10 FOR I = 1 TO 3
> 20 PRINT I, I * I
> 30 NEXT I
> RUN
1 1
2 4
3 9
```

## Try it

Install QEMU (`sudo apt install qemu-system-arm` on Ubuntu or Debian), download
`homecore-vX.Y.Z-lm3s6965evb.elf` from the latest
[release](https://github.com/grzegorz-grzeda/home-core/releases), and run it:

```bash
qemu-system-arm -M lm3s6965evb -nographic -kernel homecore-vX.Y.Z-lm3s6965evb.elf
```

At the `root:/$` prompt, type `help`. Quit QEMU with Ctrl-A, then X. The
[ten-minute tour](docs/tour.md) walks through a first session, and the
[BASIC guide](docs/basic.md) shows how to write programs.

## What it can do

| | |
| --- | --- |
| ✓ Shell | `ls`, `cd`, `cat`, `cp`, `write`, `mkdir`, `rm`, and more, with per-session working directories |
| ✓ Files | One file tree for RAM files, devices (`/dev/uart0`, `/dev/led0`, `/dev/uptime`), and mounted disks |
| ✓ Disks | FAT and littlefs volumes; littlefs survives resets during writes |
| ✓ BASIC | Line-numbered programs with `FOR`, `IF`, `GOSUB`, plus `millis()`, `led()`, and `ledget()` |
| ✓ Hardware | UART console, GPIO, LEDs, all described in YAML per board |
| ✓ Tested | Every change is built for each board and run in QEMU; the guides' examples are checked there too |
| Planned | SD cards and permanent storage, loading programs from disk, multitasking |

HomeCore runs one program at a time, and everything is currently kept in
memory, so files are lost at reset. There are no user passwords or
permissions yet.

## Boards

| Board | Chip | Status |
| --- | --- | --- |
| [LM3S6965EVB](docs/boards/lm3s6965evb.md) | Stellaris Cortex-M3 | In QEMU only; tested in CI |
| [STM32F4DISCOVERY](docs/boards/stm32f4discovery.md) | STM32F407, Cortex-M4F | Builds in CI; not yet run on the board |
| [STM32VLDISCOVERY](docs/boards/stm32vldiscovery.md) | STM32F100, Cortex-M3, 8 KB RAM | Tested in QEMU in CI; not yet run on the board |

## Build it yourself

Install CMake 3.25+, Ninja, the Arm GNU bare-metal toolchain, Python 3 with
venv support, and QEMU with Arm system emulation. Then, from the repository
root:

```bash
git submodule update --init --recursive
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
cmake --preset lm3s6965evb -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset lm3s6965evb
bash scripts/run-qemu-lm3s6965evb.sh
```

For flashing and wiring a real board, see its board guide above. The
[development guide](docs/development.md) covers every preset, the tests, and
debugging.

## Documentation

**Using HomeCore**
- [A first session](docs/tour.md): a guided ten-minute tour.
- [BASIC guide](docs/basic.md): the language, LEDs, timing, and examples.
- [Shell and system services](docs/shell.md): every command in detail.
- [Board guides](docs/boards/): running HomeCore on each board.

**Working on HomeCore**
- [Development](docs/development.md): setup, builds, configuration, tests, and debugging.
- [Architecture](docs/architecture.md): layers, startup, memory, and runtime contracts.
- [Extending](docs/extending.md): shell commands, device files, drivers, and filesystems.
- [Porting](docs/porting.md): adding boards, chips, and CPU architectures.
- [API reference](https://grzegorz-grzeda.github.io/home-core/): the public headers, the
  driver and storage guides, and the bundled G2Basic reference, published from `main`.
- [C coding standard](docs/coding-standard.md), [versioning](docs/versioning.md), and the
  [changelog](CHANGELOG.md).
- [Contributor and agent instructions](AGENTS.md) ([CLAUDE.md](CLAUDE.md) imports them).

## License

HomeCore is MIT licensed; see [LICENSE](LICENSE). Created by Grzegorz Grzęda.
Board photos in `docs/assets/boards` are CC BY-SA; see
[photo credits](docs/assets/boards/PHOTOS.md).
External dependencies keep their own licenses, including the vendored
[STM32 device headers](external/stm32f4/README.md) and
[FatFs](external/fatfs/README.md) (ChaN's BSD-style one-clause license), and the
[littlefs](https://github.com/littlefs-project/littlefs) submodule (BSD-3-Clause).
