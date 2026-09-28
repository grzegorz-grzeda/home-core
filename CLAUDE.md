# CLAUDE.md

The repository rules live in AGENTS.md and are imported here. They are binding,
including the mandatory coding-standard review and completion-report format.

@AGENTS.md

## Quick reference

HomeCore is single-threaded C11 bare-metal firmware: a serial shell, VFS with RAM
files, sessions, uptime, and G2BASIC. There is no working scheduler, privilege
boundary, persistence, or authentication (SVC/PendSV panic). Do not describe
these as implemented.

| Task | Guide |
| --- | --- |
| Layers, startup order, memory, I/O path | [docs/architecture.md](docs/architecture.md) |
| Setup, builds, Kconfig, tests, CI, debugging | [docs/development.md](docs/development.md) |
| Coding rules | [docs/coding-standard.md](docs/coding-standard.md) |
| Shell commands and system services | [docs/shell.md](docs/shell.md) |
| New shell commands and VFS device nodes | [docs/extending.md](docs/extending.md) |
| New boards, SoCs, CPUs | [docs/porting.md](docs/porting.md) |
| Board specifics | [docs/boards/](docs/boards/) |

Commands, run from the repository root with the project venv:

```bash
cmake --preset lm3s6965evb -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset lm3s6965evb
python3 tests/qemu_files_test.py                                # LM3S QEMU regression
.venv/bin/python scripts/check_quality.py --checks format headers host
.venv/bin/python scripts/check_quality.py                       # full run incl. both boards + QEMU
doxygen Doxyfile                                                # API docs -> build/docs; fails on warnings
```

## Notes for Claude Code

- Use `.venv/bin/python` and `.venv/bin/clang-format` (19.1.7). A missing
  `kconfiglib` error means CMake picked the system Python.
- Host tests include the generated `build/lm3s6965evb/include/homecore/autoconf.h`;
  configure the LM3S preset before compiling them by hand.
- Firmware changes are validated in QEMU (LM3S) only. Never report STM32
  behavior as verified from a build; say that hardware validation is pending.
- `external/cmsis`, `external/g2basic`, and `external/doxygen-awesome-css` are
  submodules. Do not edit them in place. Do not edit anything under `build/`.
- Public headers carry Doxygen comments grouped by layer (`docs/doxygen/groups.dox`).
  When changing a public declaration, update its comment in the same change.
- The QEMU shell is interactive (`bash scripts/run-qemu-lm3s6965evb.sh`, exit with
  Ctrl-A X). For non-interactive checks, extend `tests/qemu_files_test.py` instead.
