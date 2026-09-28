<p align="center">
  <img src="docs/assets/homecore-logo-wordmark.svg" alt="HomeCore" width="360">
</p>

HomeCore is a small bare-metal personal-computer project for STM32, developed
with a QEMU target. It currently runs a serial shell, RAM files, a VFS, a root
session, and the G2BASIC interpreter in a single-threaded firmware image.

## Supported targets

| Target | CPU | Status |
| --- | --- | --- |
| [lm3s6965evb](docs/boards/lm3s6965evb.md) | Cortex-M3 | QEMU build and command regression tested |
| [stm32f4discovery](docs/boards/stm32f4discovery.md) | STM32F407 Cortex-M4F | Cross-compiled; physical-board validation pending |
| [stm32vldiscovery](docs/boards/stm32vldiscovery.md) | STM32F100 Cortex-M3, 8 KB RAM | QEMU regression tested; physical-board validation pending |

## Quick start

Install CMake 3.25+, Ninja, Arm GNU bare-metal tools, Python 3 with venv support,
and QEMU with Arm system emulation. Run these commands from the repository root:

```bash
git submodule update --init --recursive
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
cmake --preset lm3s6965evb -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset lm3s6965evb
bash scripts/run-qemu-lm3s6965evb.sh
```

At the `root:/$ ` prompt, try `help`, `mem`, `uptime`, `mkdir /tmp`,
`touch /tmp/notes`, `ls /tmp`, or `basic`. Exit QEMU with Ctrl-A, then X.
For STM32 flashing and UART wiring, follow the board guide above.

## Current scope

Implemented: UART console, shell commands, libc I/O and heap integration,
RAM files/directories, per-session working directories, user identity,
1 kHz SysTick uptime, and BASIC with `millis()`.

Still planned: persistent storage, authentication and permissions, a scheduler
and working thread switches, synchronization, a protected SVC syscall ABI,
and additional device drivers. SVC and PendSV handlers currently panic;
context-switch helper functions do not constitute working scheduling.
RAM files and session state are lost on reset.

## Documentation

- [Architecture](docs/architecture.md): layers, startup, memory, and runtime contracts.
- [C coding standard](docs/coding-standard.md): language, style, memory, and interrupt rules.
- [Development](docs/development.md): setup, builds, configuration, tests, and debugging.
- [Automated quality checks](docs/development.md#automated-quality-check): run `scripts/check_quality.py`.
- [API reference](https://grzegorz-grzeda.github.io/home-core/): grouped HTML docs of the public headers,
  published from `main` by CI, with the bundled G2Basic reference. [Generate them locally](docs/development.md#api-documentation)
  with `bash scripts/build_docs.sh`.
- [Porting](docs/porting.md): adding boards, SoCs, and architecture support.
- [Shell and system services](docs/shell.md): commands, sessions, RAM files, and uptime.
- [Extending](docs/extending.md): adding shell commands and VFS device nodes.
- [Versioning](docs/versioning.md) and [changelog](CHANGELOG.md): SemVer 2.0.0 rules and release history.
- [Contributor and agent instructions](AGENTS.md): repository working conventions
  ([CLAUDE.md](CLAUDE.md) imports them for Claude Code).

## License

HomeCore is MIT licensed; see [LICENSE](LICENSE). Created by Grzegorz Grzęda.
External dependencies retain their own licenses, including the vendored
[STM32 device headers](external/stm32f4/README.md).
