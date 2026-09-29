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
are submodules, as is the doxygen-awesome-css documentation theme. ST's device
headers are vendored with their license and version.

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
`configs/<board>_defconfig` when the board provides one, otherwise
`configs/homecore_defconfig`. To use another input, copy a
defconfig, edit its `CONFIG_...` settings, and configure with
`-DHOMECORE_DEFCONFIG=/absolute/path`. The choice is cached per build directory.
Rerun configuration after changing Kconfig or defconfig; generation happens at
configure time. Editing build-directory `.config` does not persist changes.

Useful settings include main stack size, stdio buffering, shell input/argument limits and prompt
suffix, VFS open-file/directory/file limits, and maximum RAM-file size.
Use the Kconfig files for current defaults and ranges. Disabling shell or VFS
is not a validated minimal-system configuration: startup references them directly.
G2BASIC options such as `G2BASIC_ENABLE_MATH_FUNCTIONS` and `G2BASIC_ENABLE_MATH`
are CMake options, separate from HomeCore Kconfig.

## Device description

Hardware is described in YAML and turned into C at configure time; nothing is
parsed at run time. Three layers are merged in order, later ones winning per
key:

| Layer | File | Contents |
| --- | --- | --- |
| SoC | `src/soc/<vendor>/<chip>/soc.yaml` | `arch`, `memory` (flash and RAM base and size), and every peripheral instance, `status: disabled` |
| Board | `src/board/<board>/board.yaml` | `name`, `soc`, `clocks` (`cpu` plus named buses), enabled devices and their properties, `memory` overrides, `chosen.console`, `mounts` |
| Overlays | files in `HOMECORE_DT_OVERLAYS` | changes to `memory`, `clocks`, `devices`, `chosen`, or `mounts`, for example `-DHOMECORE_DT_OVERLAYS=path/to/debug.yaml` |

The board may also set `clock_setup: false` (usually from an overlay) for
emulators that do not model the clock controller; boards then skip their
clock setup (`DT_CLOCK_SETUP` is 0). The `stm32vldiscovery-qemu` preset applies
`src/board/stm32vldiscovery/qemu.yaml` this way.

The board selects the SoC and the SoC selects the architecture; a layer that
sets a key it does not own (`arch` in a board, `clocks` in a SoC, `soc` in an
overlay) is rejected. `name` defaults to the board directory name and becomes
`DT_BOARD_NAME` and the `BOARD` macro.

```yaml
# soc.yaml
devices:
  usart1: {compatible: "st,stm32-usart", reg: 0x40013800, irq: 37, bus: apb2, status: disabled}
# board.yaml
clocks: {cpu: 24000000, apb1: 24000000, apb2: 24000000}
devices:
  usart1: {status: okay, baud: 115200, devname: uart0}
chosen: {console: usart1}
```

Quote `compatible` strings inside `{...}`: YAML reads the comma as a separator.

A board can also define devices that are not SoC peripherals, such as a
ramdisk, and mount filesystems on block devices. Each `mounts` entry is a
top-level path; `format: true` creates a volume when the device has none
(default `false`, so an SD card is never reformatted by accident):

```yaml
devices:
  ram0: {compatible: "homecore,ramdisk", size: 64K}
  ram1: {compatible: "homecore,ramdisk", size: 16K}
mounts:
  /ram: {device: ram0, fs: fat, format: true}
  /lfs: {device: ram1, fs: littlefs, format: true}
```

The generated `dt_mount_all()`, called by `main()` after `k_init()`, mounts
them in order and prints `mount <path>: <error>` for a failure instead of
stopping. `fs` is `fat` or `littlefs`; each library is compiled only when a
board mounts one of its volumes. FAT volumes need at least 128 sectors
(64 KB), littlefs volumes 4 sectors. The API reference's
[Files and storage](https://grzegorz-grzeda.github.io/home-core/storage.html)
page compares them.

Each compatible has a binding next to its driver,
`src/drivers/<class>/<compatible>.yaml`. It names the driver prefix and sources
and declares every property: its type (`int`, `string`, `bool`, `clock` for a
bus name resolved through `clocks`, `size` for a byte count such as `64K`,
`gpio` for the node name of an enabled GPIO port, or `devpath` for a `/dev`
name that defaults to the node name), whether it is required, its default,
minimum, `maximum` (`int`), and required `multiple` (`size`), whether it must
be unique among enabled devices (`unique-with: <property>` makes it unique
per value of another property, such as a pin per port), and the C
configuration field it fills. A `gpio` property fills a `gpio_port_t`, and the
referenced port is initialized before the device that uses it. A `size` property with `buffer: <field>` also allocates a 4-byte
aligned `.bss` buffer of that size and stores its address in `<field>`.
Configuration fails, naming the device and property, for an unknown
compatible or property, a missing required property, a wrong type, a duplicate
unique value or `/dev` name, an unknown clock, a console that is disabled or
not console-capable, a `gpio` reference to a device that is not an enabled
GPIO port, and a mount on a device that is not an enabled block device, is
already mounted, or names an unknown filesystem.

Bindings mark what a driver provides: `console: true` (`<driver>_console_ops`),
`gpio: true` (`<driver>_gpio_ops`), `block: true` (`<driver>_block_ops`),
`led: true` (`<driver>_led_ops`, numbered in `dt_led_table`), and `isr: true`
(`<driver>_isr()`).

`scripts/devicetree_generate.py` writes into the build directory:

| Output | Contents |
| --- | --- |
| `include/homecore/devicetree.h` | `DT_BOARD_NAME`, `DT_CPU_CLOCK_HZ`, `DT_CLOCK_SETUP`, `DT_CHOSEN_CONSOLE_PATH`, `DT_DEVICE_COUNT`, `DT_MOUNT_COUNT`, `DT_LED_COUNT`, `dt_init()`, `dt_irq_dispatch()`, `dt_mount_all()` |
| `generated/devicetree.c` | per device a `static const <driver>_config_t` (flash) and a `<driver>_t` instance (RAM), `dt_console`, `dt_led_table`, `dt_init()`, `dt_irq_dispatch()`, a `block_device_t` per mounted device, and `dt_mount_all()` |
| `generated/memory.ld` | the linker `MEMORY` block |
| `generated/devicetree.cmake` | the selected SoC and architecture, linker fragment paths, input files, driver sources and include directories, and the mounted filesystem types |

Only drivers with an enabled device are compiled. Editing a description,
binding, or the generator reruns configuration on the next build. Driver
conventions are in [extending](extending.md#device-drivers); the generator's
tests are `tests/devicetree_generate_test.py`.

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
- Builds every target in Debug and Release with compile warnings treated as
  errors: the three boards plus `stm32vldiscovery-qemu`.
- Runs the eight host C regression variants and the runner's own Python tests.
- Runs QEMU regressions on the newly built LM3S and `stm32vldiscovery-qemu`
  configurations.

Builds use `build/quality/<target>/<configuration>` and each target's defconfig,
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
Run the relevant tests below, or all of them for shared VFS/session changes:

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

# --wrap=calloc lets the test fail chosen VFS allocations (GNU ld).
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include -Wl,--wrap=calloc \
  tests/vfs_alloc_failure_test.c src/subsystems/vfs/vfs.c -o /tmp/homecore-alloc-failure-test
/tmp/homecore-alloc-failure-test

cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  -Isrc/subsystems/shell/builtin tests/session_test.c src/kernel/session.c \
  src/subsystems/vfs/vfs.c src/subsystems/shell/shell.c \
  src/subsystems/shell/builtin/builtin_files.c \
  src/subsystems/shell/builtin/builtin_session.c \
  src/subsystems/shell/builtin/builtin_system.c -o /tmp/homecore-session-test
/tmp/homecore-session-test

# FAT volumes on ramdisks through the VFS, with the vendored FatFs.
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  -Isrc/subsystems/fs/fat -Iexternal/fatfs -Isrc/drivers/block \
  tests/fat_test.c src/subsystems/vfs/vfs.c src/subsystems/fs/fat/fat.c \
  src/subsystems/fs/fat/diskio.c src/drivers/block/ramdisk.c \
  external/fatfs/ff.c external/fatfs/ffsystem.c external/fatfs/ffunicode.c \
  -o /tmp/homecore-fat-test
/tmp/homecore-fat-test

# littlefs volumes through the VFS, including simulated resets during writes.
# The -D options match the firmware build (src/subsystems/fs/CMakeLists.txt).
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  -Isrc/subsystems/fs/littlefs -Iexternal/littlefs -Isrc/drivers/block \
  -DLFS_NAME_MAX=127 -DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR \
  tests/littlefs_test.c src/subsystems/vfs/vfs.c \
  src/subsystems/fs/littlefs/littlefs.c src/drivers/block/ramdisk.c \
  external/littlefs/lfs.c external/littlefs/lfs_util.c -o /tmp/homecore-littlefs-test
/tmp/homecore-littlefs-test

# LEDs: numbering, API, /dev files, active-low, over a fake GPIO port.
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include -Isrc/drivers/led \
  tests/led_test.c src/drivers/led/led.c src/drivers/led/gpio_led.c \
  src/drivers/led/console_led.c src/subsystems/vfs/vfs.c -o /tmp/homecore-led-test
/tmp/homecore-led-test

# STM32 GPIO drivers against fake registers; add -DTEST_STM32F1 with the
# stm32f1 fake for the F1 driver.
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  -Itests/fakes/stm32f4 -Isrc/drivers/gpio tests/gpio_contract_test.c \
  -o /tmp/homecore-gpio-test
/tmp/homecore-gpio-test
```

After building the QEMU presets, run the integration regression on each
emulated board:

```bash
python3 tests/qemu_files_test.py
python3 tests/qemu_files_test.py --board stm32vldiscovery
```

It uses `build/lm3s6965evb/homecore` or `build/stm32vldiscovery-qemu/homecore`
unless `--firmware` names another ELF. It checks shell/system/file commands,
sessions, repeated create/remove cycles, BASIC at and just past the board's
nesting limit, the console LEDs through `/dev/ledN`, `write`, and BASIC's
`led()`/`ledget()`, the littlefs volume at `/ram` (LM3S), the stack high-water
mark from `mem`, and reboot. Host tests exercise portable logic with stubs,
including the STM32 GPIO drivers against fake registers; QEMU tests exercise
the emulated firmware. Neither validates STM32 peripheral behavior or GPIO
LEDs, which QEMU does not model.

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
Eight matrix jobs visibly configure and build firmware with CMake, one per
target and build type: `Build <target> (Debug)` and `Build <target> (Release)`
for `lm3s6965evb`, `stm32f4discovery`, `stm32vldiscovery`, and
`stm32vldiscovery-qemu`.

Each build treats compiler warnings as errors and reports firmware size. The `lm3s6965evb` and `stm32vldiscovery-qemu`
jobs also run QEMU against their own freshly built ELF. Jobs run independently
so a quality failure does not hide build results. CI build output is under
`build/ci/<target>/<configuration>`.

The `API documentation` job fetches the theme and G2Basic submodules (not
CMSIS), installs Doxygen and Graphviz, and runs `scripts/build_docs.sh`. It
fails on any documentation warning in either project. It uploads the HTML
as a `github-pages` artifact, which pull-request runs keep as a downloadable
preview. On pushes to the default branch, `Publish API documentation` deploys
that artifact to GitHub Pages at <https://grzegorz-grzeda.github.io/home-core/>.
It requires Pages to be enabled once under repository **Settings → Pages →
Build and deployment → Source: GitHub Actions**. Pull requests and other
branches never deploy.

A failed check fails the job. Firmware is validated but never uploaded or
deployed; only the API documentation is published. GitHub branch protection/rulesets must require
`Quality checks`, `API documentation`, and all eight build checks if merging should be blocked by any
failure; that repository setting is separate from the workflow. Physical STM32 validation and semantic
review remain manual.

## UART and console regression tests

These host tests run the real serial drivers (`src/drivers/serial/`) against
register blocks in plain memory, supplied by the stand-in device headers in
`tests/fakes/`, plus libc length validation and console startup failure cleanup:

```bash
cc -Wall -Wextra -Werror -Iinclude -Itests/fakes/stellaris -Isrc/drivers/serial \
  tests/uart_contract_test.c -o /tmp/hc-uart-lm3s
timeout 5 /tmp/hc-uart-lm3s
cc -Wall -Wextra -Werror -Iinclude -DTEST_STM32 -Itests/fakes/stm32 -Isrc/drivers/serial \
  tests/uart_contract_test.c -o /tmp/hc-uart-stm32
timeout 5 /tmp/hc-uart-stm32
cc -Wall -Wextra -Werror -Wno-deprecated-declarations -ffunction-sections -fdata-sections -Iinclude \
  -Ibuild/lm3s6965evb/include tests/console_io_test.c -Wl,--gc-sections -o /tmp/hc-console
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
Doxygen 1.9.8 or later, Graphviz `dot`, and the submodules. On Ubuntu, install
the tools with `sudo apt-get install doxygen graphviz`. Build the full site,
including the bundled G2Basic reference, from the repository root:

```bash
git submodule update --init --recursive
bash scripts/build_docs.sh
```

Open `build/docs/index.html`. The script takes an optional output directory
(default `build/docs`) and a `DOXYGEN` variable naming the executable. It stamps
each page header with the `project()` version and short commit, unless
`HOMECORE_DOCS_VERSION` or `G2BASIC_DOCS_VERSION` is set. The output includes
include-dependency graphs for each header, collaboration graphs for structures,
and a group hierarchy graph for each module.

### Board pages

The site's **Supported boards** page and one page per board live in
`docs/doxygen/boards/`. They summarize each board's status, specifications,
console wiring, and memory map, and link to the full guide in `docs/boards/`.
When a board's support, pins, clock, memory limits, or validation status
changes, update both its Markdown guide and its page.

The diagrams are SVG files in `docs/assets/boards/`, inlined with
`@htmlinclude[block] <file>.svg` (the directory is the Doxyfile's
`EXAMPLE_PATH`). Each SVG sets light colours as presentation attributes, so it
also reads correctly when opened on its own, and tags elements with `hc-*`
classes (`hc-panel`, `hc-accent`, `hc-wire`, `hc-text`, `hc-muted`, and so on)
that `homecore.css` recolours for light and dark mode. Keep a `<title>` and
`<desc>` in each file for screen readers, and make their `id` values unique
across the site.

Doxygen 1.9.8 closes an HTML `<div>` at a blank line or a Markdown table. Inside
the card and panel layouts (`hc-board-grid`, `hc-card`, `hc-board-header`,
`hc-spec-panel`), use HTML tables and lists without blank lines. Put Markdown
tables and code blocks outside them.

### Other ways to build

For a quick check of HomeCore's headers alone, `doxygen Doxyfile` still works
and needs only the theme submodule. It omits the G2Basic reference and its
main-page section. Set `HOMECORE_DOCS_VERSION` to show a version.

When Doxygen and `dot` are found at configure time, each CMake build directory
also has a `docs` target that runs the script into `<build dir>/docs`:

```bash
cmake --build --preset lm3s6965evb --target docs
```

Without Doxygen, `dot`, or the submodules, configuration reports that the
target is disabled and firmware builds are unaffected. Rerun configuration after
installing them.

### Bundled library references

- **G2Basic** is built into `<output>/g2basic` from the pinned submodule commit,
  using G2Basic's own Doxyfile and theme. That run writes a tag file,
  `g2basic.tag`, next to the output directory. HomeCore's run reads it, so
  G2Basic groups appear on the Topics page (marked `[external]`, without
  descriptions) and `@ref` to G2Basic groups or functions links into the nested
  reference. The main-page section is enabled with `ENABLED_SECTIONS = g2basic`.
  Neither Doxyfile is modified; the script appends overrides on standard input.
- **CMSIS** is not built. Its Doxygen build needs exactly Doxygen 1.9.6 and
  `mscgen`, and HomeCore's public headers do not expose CMSIS types. The main
  page links Arm's documentation for the pinned release. After updating
  `external/cmsis`, update the version in that link in
  `docs/doxygen/groups.dox`. If public docs start referencing CMSIS symbols,
  Arm publishes a matching tag file at
  `https://arm-software.github.io/CMSIS_6/v<version>/Core/cmsis_core_m.tag`.

### Theme

The site uses [doxygen-awesome-css](https://github.com/jothepro/doxygen-awesome-css)
v2.5.0, pinned as the `external/doxygen-awesome-css` submodule (MIT license).
It supports Doxygen 1.9.6 to 1.18.0 and requires `HTML_COLORSTYLE = LIGHT`. It
provides the responsive layout, a light/dark toggle next to the search box, and
inverted Graphviz graphs in dark mode. The toggle follows the system theme
until a reader chooses one.

- `docs/doxygen/homecore.css` sets the teal palette for both modes. Keep link
  text and text on the primary colour at a WCAG contrast of at least 4.5:1;
  the file records the current ratios.
- `docs/doxygen/header.html` is Doxygen 1.9.8's default header, generated with
  `doxygen -w html header.html footer.html style.css`, plus the toggle script.
  It also renders correctly with Doxygen 1.12. When CI's Doxygen version
  changes, regenerate it the same way and re-add the two script tags.
- To update the theme, check out a newer release tag in the submodule, then
  check both modes and the phone layout before committing.

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
