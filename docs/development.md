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

The existing workflow builds LM3S Debug and Release. It does not build STM32
or run host/QEMU regressions. Its artifact glob includes `.elf`, while the
actual ELF is named `homecore`, so the ELF is not currently included by that
pattern. These are workflow gaps; local checks above remain necessary.
