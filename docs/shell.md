# Shell and system services

## Shell and BASIC

New to HomeCore? Start with [the tour](tour.md); this page is the reference.

Use `help` to list available commands. Enter `basic`
to start G2BASIC; at its input prompt, Ctrl-C, Ctrl-D, or Ctrl-Z returns to the
shell. The [BASIC guide](basic.md) teaches the language with examples. For interpreter syntax, see the [G2BASIC documentation](../external/g2basic/README.md).
One line may nest parentheses, unary signs, function calls, and `IF ... THEN`
up to `CONFIG_HOMECORE_SHELL_BASIC_MAX_NESTING` levels (8 by default); deeper
lines fail with "expression too deeply nested", which keeps BASIC within the
main stack.

## LEDs

Boards number their LEDs from 0, and each has a device file, `/dev/ledN`.
Reading it gives `0` or `1`; writing `on`, `off`, `1`, `0`, or `toggle` sets
it. From the shell, use `write`, since the shell has no output redirection:

```text
write /dev/led0 on
cat /dev/led0
write /dev/led0 toggle
```

BASIC controls the same LEDs by number:

```basic
PRINT led(0, 1)
PRINT ledget(0)
```

`led(n, state)` lights LED `n` for a nonzero `state` and turns it off for 0,
returning the new state. `ledget(n)` returns 1 if the LED is lit and 0 if not.
Both return -1 for a number without an LED, including fractions.

| Board | LEDs |
| --- | --- |
| STM32F4DISCOVERY | `led0` green (PD12), `led1` orange (PD13), `led2` red (PD14), `led3` blue (PD15) |
| STM32VLDISCOVERY | `led0` green (PC9), `led1` blue (PC8); in QEMU, console LEDs |
| LM3S6965EVB (QEMU) | `led0`, a console LED |

QEMU models no LEDs, so emulated boards use console LEDs: each change request
prints a line such as `[led0] on` on the console, and reads return the last
state set. LED states are not kept across reboots; every LED starts off.

## Uptime

The kernel starts a 1 kHz SysTick during initialization. `k_uptime_ms()` returns
a 64-bit millisecond count; BASIC exposes the same count as `millis()`,
independently of its math configuration:

```basic
print millis()
```

Open `/dev/uptime` read-only to capture uptime as decimal milliseconds followed
by a newline (for example, `12345\n`). Each open has an independent snapshot
and read position. Partial reads are supported, followed by EOF. Close and
reopen to capture a fresh value; seeking replays the existing snapshot.

```c
FILE *file = fopen("/dev/uptime", "r");
if (file) {
    char line[32];
    if (fgets(line, sizeof(line), file)) {
        fputs(line, stdout);
    }
    fclose(file);
}
```

Uptime begins when the timer starts during kernel initialization, not at reset entry.
Interrupt masking across multiple ticks can lose elapsed time. Clock accuracy and
emulation behavior are described in the [board guides](../README.md#boards).

## File commands

The shell supports:

```text
ls
ls /dev
mkdir /tmp
mkdir /tmp/a /tmp/b
ls /tmp
cat /dev/uptime
```

`ls [path]` lists direct directory children or the named file. Directory names
end with `/`. `mkdir path...` creates empty RAM directories; parents must
already exist. They disappear on reboot. The default limit is 16 user
directories, configured by `CONFIG_HOMECORE_VFS_MAX_DIRECTORIES`.

Paths allow up to 127 bytes and support repeated slashes, `.` and `..`.
Shell paths resolve relative to the session working directory. There is no
persistent file storage yet.

On boards with a mounted filesystem (`/ram` on LM3S6965EVB and
STM32F4DISCOVERY, `/lfs` on STM32F4DISCOVERY), the file commands work the same
below the mount point, and `cp` copies between filesystems:

```text
cd /ram
mkdir notes
touch notes/todo.txt
cp /dev/uptime boot-time
ls
```

Mount points cannot be removed, and a full volume fails writes with "No space
left on device". FAT names (STM32F4DISCOVERY `/ram`) are case-insensitive,
and names longer than 8.3 need `CONFIG_HOMECORE_FS_FAT_LFN` (enabled by
default); littlefs names are case-sensitive, like RAM files. Every current
volume is a ramdisk formatted at boot, so its files are lost on reboot.
`mkdir -p` and other command options are not implemented.

`cat path...` copies existing readable VFS nodes to the console until EOF.
Streaming devices such as UARTs can wait indefinitely because they do not
provide file EOF. Shell redirection is not implemented.

## Users and shell sessions

The default session runs as `root` (UID 0, GID 0), with home and working directory
`/`. The prompt includes the identity and CWD, for example `root:/dev$ `.
`CONFIG_HOMECORE_SHELL_PROMPT` configures the suffix after those fields.

```text
whoami
id
pwd
cd /dev
ls
cat uptime
cd ..
cd
```

`cd` without arguments returns to the user's home. A failed directory change
leaves CWD unchanged. `ls` without arguments lists CWD. Relative paths in
`mkdir`, `cat`, and libc `open`/`fopen` also use the active session's CWD.

`user_t` holds UID, primary GID, name, and home. `session_t` holds a user pointer,
CWD, and last command status; `shell_context_t` aliases it. Command handlers
receive that context explicitly. `shell_execute_line(context, writable_line)`
sets the active session during dispatch and restores it afterward. Unknown
commands set status 127; excessive arguments set status 2. Empty input preserves
the previous status. User objects must outlive their sessions.

The VFS has no global CWD: `vfs_resolve_path(base, path, output)` accepts an
explicit base, while existing low-level VFS operations keep their root-relative
behavior. The current-session pointer used by libc is single-threaded and must
become task-local when scheduling is added. This is identity and session state,
not authentication or access control: there are no passwords, ownership checks,
or privilege separation yet.

## System commands and files

| Command | Behavior |
| --- | --- |
| `mem` | Heap total, allocated bytes, reusable allocator space, unclaimed space, available bytes, and the main-stack high-water mark |
| `uptime` | Elapsed days and hours:minutes:seconds.milliseconds |
| `clear` | Clear an ANSI terminal and move the cursor home |
| `reboot` | Reset the board; RAM files, directories and session state are lost |
| `rmdir path...` | Remove empty user-created directories and free their memory |
| `touch path...` | Create empty files, preserving existing file contents |
| `rm path...` | Remove closed files and free their memory |
| `cp source target` | Copy a readable file or snapshot device to a new or truncated file; a directory target keeps the source's name |
| `write path text...` | Write the words, joined by spaces and ending in a newline, to a file (created or truncated) or device in one write; up to 126 characters plus the newline |

`mem` reports the heap region, excluding static RAM and reserved stack space.
Its last line, `Stack: used N of M bytes`, is the deepest main-stack use since
reset, including interrupt handlers. `N` equal to `M` means the stack was
exhausted and has probably overwritten heap memory.
Allocated bytes include allocator overhead; available bytes combine reusable
allocator blocks and unclaimed heap space, not necessarily one contiguous block.

`rmdir` rejects `/`, `/dev`, mount points, non-empty directories, and the calling shell's CWD.
`rm` rejects directories, device nodes and files that still have open descriptors.
There are no recursive-removal flags. `touch` does not maintain timestamps yet.

Each RAM file or directory is allocated from the heap when created, sized to
its path, and freed when removed; file contents are allocated only when
written. The defaults cap the VFS at 16 files
(`CONFIG_HOMECORE_VFS_MAX_RAM_FILES`), 16 directories, and 4096 bytes per file
(`CONFIG_HOMECORE_VFS_MAX_FILE_SIZE`). Reaching a cap reports "No space left on
device"; an exhausted heap reports "Not enough space". Contents are available
through VFS read/write/seek and libc `fopen`/`fread`/`fwrite`, including append and
truncate modes. Seeking past EOF and then writing fills the gap with zero bytes.

```text
mkdir /tmp
cd /tmp
touch notes
ls
cat notes
rm notes
cd /
rmdir /tmp
mem
uptime
```

See [development](development.md#validation) for regression commands.
