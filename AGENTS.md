# Repository instructions

These instructions apply throughout this repository. Read the relevant source
and documentation before changing behavior; the implementation is the authority
when documentation and code disagree.

## Project map

HomeCore is C11 bare-metal firmware using CMake, Kconfig, CMSIS, newlib-nano,
and the G2BASIC submodule. Start with [architecture](docs/architecture.md) and
[development](docs/development.md). Board-specific instructions are under
`docs/boards/`; new target work follows [porting](docs/porting.md).

## Working conventions

- Keep architecture mechanisms in `src/arch`, chip-specific code in `src/soc`,
  and board wiring/clock policy in `src/board`. Keep public interfaces in
  `include/homecore` and portable shell/VFS code in `src/subsystems`.
- Follow `.clang-format` and neighboring code: four-space indentation, attached
  braces, and a 100-column limit. Keep formatting changes scoped to edited code.
- Preserve license notices. Treat `external/cmsis` and `external/g2basic` as
  submodules; make dependency updates explicit. Preserve the source version and
  license when updating vendored `external/stm32f4` headers.
- Edit Kconfig definitions or defconfig inputs, then rerun CMake configuration.
  Do not hand-edit generated headers, `.config`, linker scripts, or build output.
- Keep CPU/ABI flags consistent across firmware, assembly, libraries, and linking.
  The current ports use software floating point.
- Do not describe scheduling, authentication, persistence, or hardware validation
  as implemented unless the code and validation support that claim.
- Update the relevant guide when changing commands, interfaces, configuration,
  target support, or setup. Keep README concise and link to detailed docs.

## Validation

Use the exact setup and test commands in [development](docs/development.md).
For shared firmware/build/startup changes, configure and build both targets:

```bash
cmake --preset lm3s6965evb -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset lm3s6965evb
cmake --preset stm32f4discovery -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset stm32f4discovery
python3 tests/qemu_files_test.py
```

Run relevant host regressions for VFS, sessions, and uptime changes. For startup
or linker changes, inspect vector placement, stack/heap bounds, data alignment,
and target CPU attributes. For documentation-only changes, check links and any
new or changed executable instructions; firmware rebuilds are unnecessary unless
needed to verify those instructions.

Report what was checked and what remains unverified. A successful STM32 build
is not evidence of successful execution on physical hardware. Current CI only
builds the LM3S target; it does not replace local regression checks.
