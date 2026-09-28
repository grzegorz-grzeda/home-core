# Shell and system services

## Shell and BASIC

Use `help` to list available commands. Enter `basic`
to start G2BASIC; at its input prompt, Ctrl-C, Ctrl-D, or Ctrl-Z returns to the
shell. For interpreter syntax, see the [G2BASIC documentation](../external/g2basic/README.md).

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
emulation behavior are described in the [board guides](../README.md#supported-targets).

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

## System commands and RAM files

| Command | Behavior |
| --- | --- |
| `mem` | Heap total, allocated bytes, reusable allocator space, unclaimed space, and available bytes |
| `uptime` | Elapsed days and hours:minutes:seconds.milliseconds |
| `clear` | Clear an ANSI terminal and move the cursor home |
| `reboot` | Reset the board; RAM files, directories and session state are lost |
| `rmdir path...` | Remove empty user-created directories and reclaim their slots |
| `touch path...` | Create empty RAM files, preserving existing file contents |
| `rm path...` | Remove closed RAM files and reclaim their storage and slots |

`mem` reports the heap region, excluding static RAM and reserved stack space.
Allocated bytes include allocator overhead; available bytes combine reusable
allocator blocks and unclaimed heap space, not necessarily one contiguous block.

`rmdir` rejects `/`, `/dev`, non-empty directories, and the calling shell's CWD.
`rm` rejects directories, device nodes and files that still have open descriptors.
There are no recursive-removal flags. `touch` does not maintain timestamps yet.

RAM-file metadata uses a fixed pool; contents are allocated from the heap only
when written. Defaults are 16 files (`CONFIG_HOMECORE_VFS_MAX_RAM_FILES`) and
4096 bytes per file (`CONFIG_HOMECORE_VFS_MAX_FILE_SIZE`). Contents are available
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
