# Development

Run all commands from the repository root. Board wiring and execution details
are in the [QEMU](boards/lm3s6965evb.md) and
[STM32](boards/stm32f4discovery.md) guides.

Follow the [C coding standard](coding-standard.md) for new and changed first-party code.

## Setup

Required: CMake 3.25+, Ninja, Arm GNU bare-metal GCC/binutils with newlib-nano,
Python 3 and venv/pip support. Host tests also require a native C compiler;
QEMU tests require `qemu-system-arm`. STM32 flashing requires OpenOCD and ST-LINK.

```bash
git submodule update --init --recursive
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
```

The Python dependency list is maintained in `requirements.txt`. CMSIS and G2BASIC
are submodules; ST's device headers are vendored with their license and version.

## Builds

```bash
cmake --preset lm3s6965evb -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset lm3s6965evb
cmake --preset stm32f4discovery -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset stm32f4discovery
```

The presets use separate `build/<board>` directories. The ELF executable is
named `homecore` (without `.elf`); `homecore.bin` and `homecore.map` accompany it.
For LM3S Release, configure and build the `lm3s6965evb-release` preset.
For a separate STM32 Release directory:

```bash
cmake -S . -B build/stm32f4discovery-release -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/arm-none-eabi.cmake" \
  -DHOMECORE_BOARD=stm32f4discovery -DCMAKE_BUILD_TYPE=Release \
  -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build build/stm32f4discovery-release
```

## Configuration

`Kconfig` includes subsystem definitions under `src/`. The default input is
`configs/homecore_defconfig`. To use another input, copy that file, edit its
`CONFIG_...` settings, and configure with `-DHOMECORE_DEFCONFIG=/absolute/path`.
Rerun configuration after changing Kconfig or defconfig; generation happens at
configure time. Editing build-directory `.config` does not persist changes.

Useful settings include main stack size, shell input/argument limits and prompt
suffix, VFS open-file/directory/file limits, and maximum RAM-file size.
Use the Kconfig files for current defaults and ranges. Disabling shell or VFS
is not a validated minimal-system configuration: startup references them directly.
G2BASIC options such as `G2BASIC_ENABLE_MATH_FUNCTIONS` and `G2BASIC_ENABLE_MATH`
are CMake options, separate from HomeCore Kconfig.

## Automated quality check

Install the pinned quality tools in the existing project venv, then run:

```bash
.venv/bin/pip install -r requirements-quality.txt
.venv/bin/python scripts/check_quality.py
```

The runner works from any directory and never rewrites source files. By default it:

- Checks formatting and formatter-assisted brace insertion for every `.c`/`.h`
  under `src`, `include`, and `tests`, including untracked files. It excludes
  `external`, generated build files, and the vendor-generated LM3S CMSIS header.
- Compiles each public header in isolation with host warnings treated as errors.
- Builds both boards in Debug and Release with compile warnings treated as errors.
- Runs the seven host C regression variants and the runner's own Python tests.
- Runs QEMU regressions on both newly built LM3S configurations.

Builds use `build/quality/<board>/<configuration>` and default defconfig settings,
leaving ordinary preset build directories alone. Host binaries use temporary
folders. Missing tools, wrong formatter versions, timeouts, and failed checks
produce a nonzero exit status. Independent checks continue after failures;
failed builds never fall back to stale firmware. Commands time out after 120
seconds by default; host test execution has a 10-second limit.

For a shorter run or detailed diagnostics:

```bash
.venv/bin/python scripts/check_quality.py --checks format headers
.venv/bin/python scripts/check_quality.py --checks build host qemu --verbose
.venv/bin/python scripts/check_quality.py --help
```

`--checks` reports omitted categories explicitly and still runs prerequisites.
Use `--clang-format`, `--python`, and `--cc` to select tool executables, or
`--timeout` to adjust the command budget. This runner requires the build/test
dependencies listed above; it does not install tools automatically.

Formatting violations are reported as failures, not silently excluded.
A green automated run does not replace the mandatory semantic review
in `AGENTS.md`, static analysis of all possible behavior, or hardware validation.
The existing host `mallinfo()` deprecation exception remains limited to the
console I/O test. CI runs formatting/header/host checks through this script and
uses explicit matrix jobs for firmware builds and QEMU regressions.

## Validation

Configure the default LM3S build first: host tests include its generated header.
Run the relevant tests below, or all four for shared VFS/session changes:

```bash
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  tests/uptime_test.c src/subsystems/vfs/vfs.c -o /tmp/homecore-uptime-test
/tmp/homecore-uptime-test

cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  tests/vfs_directories_test.c src/subsystems/vfs/vfs.c -o /tmp/homecore-directories-test
/tmp/homecore-directories-test

cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  tests/vfs_ram_files_test.c src/subsystems/vfs/vfs.c -o /tmp/homecore-ram-files-test
/tmp/homecore-ram-files-test

cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  -Isrc/subsystems/shell/builtin tests/session_test.c src/kernel/session.c \
  src/subsystems/vfs/vfs.c src/subsystems/shell/shell.c \
  src/subsystems/shell/builtin/builtin_files.c \
  src/subsystems/shell/builtin/builtin_session.c \
  src/subsystems/shell/builtin/builtin_system.c -o /tmp/homecore-session-test
/tmp/homecore-session-test
```

After building LM3S, run the integration regression:

```bash
python3 tests/qemu_files_test.py
```

It expects `build/lm3s6965evb/homecore` and checks shell/system/file commands,
sessions, slot reuse, and reboot. Host tests exercise portable logic with stubs;
QEMU tests exercise the LM3S firmware. Neither validates STM32 peripheral behavior.

For architecture/linker changes, inspect the ELF and map:

```bash
arm-none-eabi-size build/stm32f4discovery/homecore
arm-none-eabi-readelf -A build/stm32f4discovery/homecore
arm-none-eabi-objdump -h build/stm32f4discovery/homecore
arm-none-eabi-nm -n build/stm32f4discovery/homecore
```

Check vector placement, initial stack/reset entries, aligned startup copy/zero
boundaries, non-overlapping heap/stack bounds, and consistent CPU/float ABI.
Use the board guide's smoke checks on physical hardware and record that result
separately from cross-compilation.

## Debugging and troubleshooting

Inspect `homecore.map` for RAM/flash usage. A missing `kconfiglib` module usually
means CMake selected the wrong Python; pass the venv interpreter explicitly.
Missing CMSIS or G2BASIC sources usually mean submodules were not initialized.

To stop QEMU at reset for GDB:

```bash
qemu-system-arm -M lm3s6965evb -kernel build/lm3s6965evb/homecore \
  -display none -monitor none -serial stdio -S -gdb tcp::1234
```

In an Arm-capable GDB, load that ELF, run `target remote localhost:1234`, set
breakpoints at `Reset_Handler` or `main`, then `continue`. For STM32, start
OpenOCD with `-f board/stm32f4discovery.cfg`, connect GDB to port 3333, and use
`monitor reset halt`. Early faults may occur before UART initialization, so
inspect fault registers and the stacked PC rather than relying on panic text.

## CI coverage

The [GitHub Actions workflow](../.github/workflows/build.yml) runs on pushes and
pull requests targeting `main` or `master`, and supports manual dispatch.
Its `Quality checks` job runs the script with `--checks format headers host`,
covering formatting, public headers, host regressions, and runner self-tests.
Four separate matrix jobs visibly configure and build firmware with CMake:

- `Build lm3s6965evb (Debug)`
- `Build lm3s6965evb (Release)`
- `Build stm32f4discovery (Debug)`
- `Build stm32f4discovery (Release)`

Each build treats compiler warnings as errors and reports firmware size. The
LM3S jobs also run QEMU against their own freshly built ELF. Jobs run independently
so a quality failure does not hide build results. CI build output is under
`build/ci/<board>/<configuration>`.

The `API documentation` job installs Doxygen and Graphviz and runs
`doxygen Doxyfile`, failing on any documentation warning. It uploads the HTML
as a `github-pages` artifact, which pull-request runs keep as a downloadable
preview. On pushes to the default branch, `Publish API documentation` deploys
that artifact to GitHub Pages at <https://grzegorz-grzeda.github.io/home-core/>.
It requires Pages to be enabled once under repository **Settings → Pages →
Build and deployment → Source: GitHub Actions**. Pull requests and other
branches never deploy.

A failed check fails the job. Firmware is validated but never uploaded or
deployed; only the API documentation is published. GitHub branch protection/rulesets must require
`Quality checks`, `API documentation`, and all four build checks if merging should be blocked by any
failure; that repository setting is separate from the workflow. Physical STM32 validation and semantic
review remain manual.

## UART and console regression tests

These host tests exercise actual UART callbacks with mocked hardware and board
I/O, plus libc length validation and console startup failure cleanup:

```bash
cc -Wall -Wextra -Werror -Iinclude tests/uart_contract_test.c -o /tmp/hc-uart-lm3s
timeout 5 /tmp/hc-uart-lm3s
cc -Wall -Wextra -Werror -Iinclude -DTEST_STM32 tests/uart_contract_test.c -o /tmp/hc-uart-stm32
timeout 5 /tmp/hc-uart-stm32
cc -Wall -Wextra -Werror -Wno-deprecated-declarations -ffunction-sections -fdata-sections -Iinclude tests/console_io_test.c -Wl,--gc-sections -o /tmp/hc-console
/tmp/hc-console
```

Section garbage collection omits unused newlib hooks from the host test link.
The deprecation exception is local to this test: host libc marks `mallinfo()`
deprecated, while the firmware uses newlib's API. The tests do not validate
physical UART timing or STM32 clock behavior.

## Formatting checks

Use clang-format 19.1.7 for reproducible checks. It names its C/C++ configuration
language `Cpp`; this does not change HomeCore's C11 compilation mode.

```bash
.venv/bin/pip install clang-format==19.1.7
rg --files src -g '*.c' | xargs .venv/bin/clang-format --dry-run --Werror
.venv/bin/clang-format --dry-run --Werror include/homecore/board/board.h tests/uart_contract_test.c tests/console_io_test.c
```

The quality runner checks all first-party C sources and headers, including host
tests, both locally and in CI. `InsertBraces` is enabled; review formatter edits
before accepting them. Passing formatting does not establish semantic compliance.

## API documentation

The public headers under `include/homecore` carry Doxygen comments. Each header
defines one module group, and each module belongs to a layer group:

| Layer group | Modules |
| --- | --- |
| Hardware abstraction (`hal`) | `arch`, `soc`, `board` |
| Kernel services (`kernel_services`) | `kernel`, `user`, `session` |
| Subsystems (`subsystems`) | `vfs`, `shell` |

The layer groups and the main page are defined in `docs/doxygen/groups.dox`.
The configuration is the repository [`Doxyfile`](../Doxyfile). It requires
Doxygen 1.9.8 or later and Graphviz `dot`. On Ubuntu, install them with
`sudo apt-get install doxygen graphviz`. Generate the HTML from the repository
root:

```bash
doxygen Doxyfile
```

Open `build/docs/index.html`. The output includes include-dependency
graphs for each header, collaboration graphs for structures, and a group
hierarchy graph for each module.

When Doxygen and `dot` are found at configure time, each CMake build directory
also has a `docs` target. It writes to `<build dir>/docs` and stamps the
`project()` version:

```bash
cmake --build --preset lm3s6965evb --target docs
```

Without Doxygen or `dot`, configuration reports that the target is disabled
and firmware builds are unaffected. Rerun configuration after installing them.

CI publishes the documentation for the default branch to
[GitHub Pages](https://grzegorz-grzeda.github.io/home-core/); see
[CI coverage](#ci-coverage).

Undocumented public declarations, undocumented parameters, and malformed
comments are warnings, and `WARN_AS_ERROR = FAIL_ON_WARNINGS` makes the run
fail on them. When adding or changing a public declaration:

- Put it inside its header's `@defgroup ... @{ ... @}` block. Use a `@name`
  member group for a related set of declarations, as in `vfs.h` and `arch.h`.
- Give it a `@brief` and document every parameter. Also document the return
  value and `errno` values, ownership and lifetime of retained pointers, and
  interrupt-context restrictions.
- A new header needs a `@file` comment and a `@defgroup` placed in a layer
  group with `@ingroup`. Add a new layer group only in `docs/doxygen/groups.dox`.
- Start license comments with `/*`, not `/**`. Doxygen treats `/**` as
  documentation and would copy the license into the file description.

Generated headers (`autoconf.h`, `version.h`) and sources under `src/` are
not part of the API reference.
