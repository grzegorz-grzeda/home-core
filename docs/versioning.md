# Versioning

HomeCore follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html).
Every change to shipped code carries a version decision, recorded in the same
change.

## Version source

The only version definition is `project(homecore VERSION X.Y.Z)` in the root
`CMakeLists.txt`. CMake generates `homecore/version.h` from it
(`HOMECORE_VERSION_STRING` and the major, minor, and patch macros). The firmware
banner prints it, and the API documentation header shows it. Never write the
version anywhere else.

CMake accepts only numeric versions. SemVer pre-release identifiers and build
metadata (`1.0.0-rc.1`, `1.0.0+abc1234`) appear only in git tags and release
names. The `project()` version is the release they lead to.

## Public interface

SemVer compatibility is judged against this interface:

- The C API in `include/homecore/`: declarations, types, macros, and the
  documented behavior, return values, and `errno` values.
- Shell commands: names, arguments, exit statuses, and documented effects.
  Output text meant for people is not part of the interface.
- VFS paths provided by the system, such as `/dev/uart0` and `/dev/uptime`,
  and their content formats.
- Kconfig symbols, their meaning, and their defaults.
- The build interface: CMake presets, `HOMECORE_BOARD` values, cache options,
  and the output file names under `build/<board>/`.
- The list of supported boards and their documented wiring.
- The BASIC language available through `basic`, as provided by the bundled
  G2Basic version.

Internal functions, source layout, linker symbols, and host test helpers are
not part of the interface.

## Choosing the increment

HomeCore is in initial development (major version 0). SemVer allows anything to
change in 0.y.z; this project narrows that with a fixed convention:

| Change | While 0.y.z | From 1.0.0 |
| --- | --- | --- |
| Incompatible change to the public interface | MINOR (0.3.1 → 0.4.0) | MAJOR |
| Backward-compatible feature, such as a new command, board, or API function | PATCH (0.3.1 → 0.3.2) | MINOR |
| Backward-compatible bug fix | PATCH | PATCH |

Resetting follows SemVer: a MINOR increment resets PATCH to 0, and a MAJOR
increment resets MINOR and PATCH. When one change contains several kinds of
change, apply only the largest increment once. Moving to 1.0.0 is a deliberate
maintainer decision, made when the public interface is considered stable.

## When to change the version

Change the version in the same commit or pull request as a change to shipped
code:

- Firmware sources and headers: `src/`, `include/`.
- Anything that changes the firmware image or its configuration: linker
  scripts, `Kconfig` files, `configs/`, CMake files that affect firmware
  compilation, and toolchain flags.
- Submodule or vendored dependency updates (`external/`) that change the
  firmware. Choose the increment by the effect on HomeCore's public interface.
  For example, a G2Basic update that removes a BASIC feature is incompatible.

These changes do not change the version: documentation, comment-only edits,
tests, CI workflows, development scripts, documentation tooling, and
formatting-only changes that leave the firmware image identical. When unsure
whether a change affects the image, compare `homecore.bin` before and after.

A version number is never reused. If a change lands without its increment, the
fix is a follow-up increment, not a rewrite of the history.

## Recording the change

Each version change adds an entry at the top of [CHANGELOG.md](../CHANGELOG.md):

```markdown
## [0.3.2] - 2026-10-05

### Added
- `date` shell command printing the uptime as days and time.
```

Use the sections `Added`, `Changed`, `Deprecated`, `Removed`, `Fixed`, and
`Security`, and omit empty ones. Describe the effect on users of the interface,
not the implementation. Mark incompatible changes with **Breaking:** at the
start of the item.

Releases are git tags named `vX.Y.Z` on the commit that sets that version.
Pre-releases use tags such as `v1.0.0-rc.1`. Tags are never moved or deleted.
