# Changelog

All notable changes to HomeCore are recorded here. The format is based on
[Keep a Changelog 1.1.0](https://keepachangelog.com/en/1.1.0/), and versions
follow [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) as
described in [versioning](docs/versioning.md).

## [0.0.2] - Baseline

Baseline for this changelog, when the versioning policy was adopted. Earlier
changes are recorded only in the git history. This version provides:

- UART shell with file, session, and system commands, and the G2Basic
  interpreter through `basic`.
- VFS with device nodes, RAM files and directories, and per-session working
  directories.
- Newlib integration for console I/O and heap, and a 1 kHz SysTick uptime clock.
- Boards: `lm3s6965evb` (QEMU-tested) and `stm32f4discovery` (cross-compiled;
  hardware validation pending).
