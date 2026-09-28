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
- Follow the [C coding standard](docs/coding-standard.md) for new and changed
  first-party code, including mandatory control-statement braces. Use
  `.clang-format` for layout and keep formatting changes scoped to edited code.
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

## Mandatory coding-standard review

Every change to first-party C code or headers, including tests, MUST undergo a
review against [the C coding standard](docs/coding-standard.md) before it is
reported complete or ready for merge. This is a required completion gate, not
an optional cleanup step. Review the final diff after implementation and checks;
re-review any subsequent code edits.

- Review all added/modified code and the surrounding contracts affected by it.
  Check braces, formatting, naming, header self-containment, visibility, types,
  conversions, bounds, ownership, resource cleanup, error handling, interrupt
  safety, hardware access, and architecture boundaries. Mark irrelevant areas
  as not applicable rather than silently skipping the review.
- Run the pinned formatter checks on changed first-party C files and headers,
  plus the builds and regressions required by the Validation section. Inspect
  formatter changes, especially around preprocessor conditionals. Passing
  formatting or tests alone does not satisfy the semantic review.
- Fix every violation introduced or exposed in the affected code path before
  declaring the change complete. Do not bypass failures with broad warning
  suppressions, weaker rules, or an unsupported compliance claim.
- A necessary low-level exception must identify the exact rule, technical reason,
  affected location, and validation evidence. Record it beside the code and in
  the completion report. Convenience or lack of time is not a valid exception.
- Report unrelated pre-existing violations separately; do not silently claim
  repository-wide compliance or perform unrelated rewrites to clear them.
- If a required check cannot run or a finding remains unresolved, explicitly
  report the gap and do not mark the review as passed or the change ready.

The completion report MUST state the review result (pass, pass with documented
exceptions, or incomplete), checks performed, and remaining findings/limitations.
Self-review is sufficient unless a separate reviewer is explicitly requested.
Documentation-only changes require link/instruction validation, not a C review.

## Validation

Use the exact setup and test commands in [development](docs/development.md).
The automated runner is `python scripts/check_quality.py`; see the guide for
its dependencies, full-check scope, and existing findings. Its result does not
replace the mandatory semantic review above.
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
is not evidence of successful execution on physical hardware. CI runs formatting,
header, and host checks plus explicit Debug/Release build jobs for both boards
and LM3S QEMU tests. It does not replace semantic review or hardware validation.
