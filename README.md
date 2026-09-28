# home-core
HomeCore — a tiny personal computer built on STM32

## Architecture 
Kernel:
- scheduler
- heap
- VFS
- syscalls
- users/sessions

System services:
- shell
- login
- BASIC interpreter

Devices:
- console
- keyboard
- display
- uart
- spi
- i2c

Filesystems:
- ramfs first
- littlefs later

## Milestones
1. Booting monitor
   - [x] boot on QEMU lm3s6965evb
   - [x] UART console
   - [x] printf
   - [x] malloc
   - [x] prompt >
   - [x] help, echo, mem, heap

2. Shell
   - [x] command parser
   - [x] argc/argv
   - [x] command registry
   - [ ] line editor
   - [ ] simple history

3. RAMFS/VFS
   - [ ] /dev/console
   - [ ] /sys/heap
   - [ ] /tmp
   - [x] ls
   - [x] cat
   - [ ] write
   - [x] rm

4. BASIC
   - [x] basic command
   - [x] PRINT
   - [x] LET
   - [x] INPUT
   - [x] GOTO
   - [x] IF
   - [x] FOR/NEXT
   - [ ] SAVE/LOAD przez VFS

5. Users
   - [ ] login
   - [ ] logout
   - [x] whoami
   - [ ] session
   - [x] home directory
   - [ ] simple permissions

6. Threads
   - [ ] idle thread
   - [ ] shell thread
   - [ ] sleep
   - [x] yield
   - [x] SysTick
   - [x] PendSV context switch
   - [ ] mutex

7. SVC/ABI
   - [ ] syscall IDs
   - [ ] SVC wrapper
   - [ ] SVC handler
   - [ ] sys_write/sys_read/sys_open
   - [ ] shell via syscall API
   - [ ] BASIC via syscall API

## Directory structure proposal
```
src/
├── main.c
├── arch/
│   └── armv7m/
│       ├── context_switch.c
│       ├── exception.c
│       ├── svc.c
│       └── include/
│           └── hc_arch.h
├── board/
│   └── qemu_lm3s6965evb/
│       ├── startup.c
│       ├── linker.ld
│       ├── board.c
│       ├── syscalls.c
│       └── include/
│           └── board.h
├── kernel/
│   ├── thread.c
│   ├── scheduler.c
│   ├── heap.c
│   ├── syscall.c
│   ├── mutex.c
│   ├── semaphore.c
│   └── include/
│       ├── hc_thread.h
│       ├── hc_heap.h
│       └── hc_syscall.h
├── vfs/
│   ├── vfs.c
│   ├── devfs.c
│   └── include/
│       └── hc_vfs.h
├── fs/
│   ├── ramfs.c
│   └── include/
│       └── hc_ramfs.h
├── drivers/
│   ├── uart/
│   ├── spi/
│   ├── i2c/
│   ├── display/
│   └── keyboard/
├── shell/
│   ├── shell.c
│   ├── commands.c
│   └── include/
│       └── hc_shell.h
├── user/
│   ├── user.c
│   ├── session.c
│   ├── auth.c
│   └── include/
│       └── hc_user.h
├── basic/
│   ├── basic.c
│   ├── lexer.c
│   ├── parser.c
│   ├── runtime.c
│   └── include/
│       └── hc_basic.h
└── libc/
    └── newlib_syscalls.c
```

## License
Created under MIT license by Grzegorz Grzęda
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

The current board clock value targets QEMU's LM3S6965EVB reset configuration
(12.5 MHz). This follows the reset divider and clock calculation in
[QEMU's Stellaris model](https://github.com/qemu/qemu/blob/master/hw/arm/stellaris.c).
Update `board_cpu_clock_hz()` if configuring a different clock or running on
physical hardware. Uptime begins when the timer starts, not at reset entry;
interrupt masking across multiple ticks can lose elapsed time. QEMU uptime
tracks guest virtual time and stops advancing when emulation is paused.

Run the host uptime/VFS regression test after configuring the firmware:

```bash
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  tests/uptime_test.c src/subsystems/vfs/vfs.c -o /tmp/uptime-test
/tmp/uptime-test
```

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

Run `python3 tests/qemu_files_test.py` after building to check these commands
in QEMU. The host directory tests can be run with:

```bash
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  tests/vfs_directories_test.c src/subsystems/vfs/vfs.c -o /tmp/vfs-test
/tmp/vfs-test
```

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

Run the session regression tests with:

```bash
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  -Isrc/subsystems/shell/builtin tests/session_test.c src/kernel/session.c \
  src/subsystems/vfs/vfs.c src/subsystems/shell/shell.c \
  src/subsystems/shell/builtin/builtin_files.c \
  src/subsystems/shell/builtin/builtin_session.c \
  src/subsystems/shell/builtin/builtin_system.c -o /tmp/session-test
/tmp/session-test
```

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

The QEMU command regression test also checks system commands and reboot.
Run the host RAM-file tests with:

```bash
cc -Wall -Wextra -Werror -Iinclude -Ibuild/lm3s6965evb/include \
  tests/vfs_ram_files_test.c src/subsystems/vfs/vfs.c -o /tmp/ram-files-test
/tmp/ram-files-test
```

## STM32F4DISCOVERY

The `stm32f4discovery` target supports the STM32F407VGT6 board with a polling
USART2 console, shell/VFS, and 1 kHz SysTick. It uses the 16 MHz internal HSI
clock, software floating point, 1 MB flash, and 128 KB main SRAM. The separate
64 KB CCM RAM is unused. USB, audio, sensors, and hardware FPU context switching
are not implemented. SVC/PendSV remain the existing panic handlers.

Build with an Arm GNU bare-metal toolchain, CMake, Ninja, and Python:

```bash
git submodule update --init --recursive
python3 -m venv .venv
.venv/bin/pip install -r requirements.txt
cmake --preset stm32f4discovery -DPython3_EXECUTABLE="$PWD/.venv/bin/python"
cmake --build --preset stm32f4discovery
```

The ELF is `build/stm32f4discovery/homecore`; the raw flash image is
`build/stm32f4discovery/homecore.bin`. With OpenOCD installed and the on-board
ST-LINK connected (CN3 jumpers fitted), program and reset with:

```bash
openocd -f board/stm32f4discovery.cfg \
  -c "program build/stm32f4discovery/homecore verify reset exit"
```

Connect a **3.3 V USB-to-UART adapter** and use **115200 baud, 8N1, no flow control**:

| Board pin | Adapter pin |
| --- | --- |
| PA2 (USART2 TX) | RX |
| PA3 (USART2 RX) | TX |
| GND | GND |

Power the board through its ST-LINK USB connector; leave the adapter power pin
disconnected. Keep BOOT0 low for flash boot. The console appears as `/dev/uart0`.
Do not assume ST-LINK provides a connected UART: VCP availability and routing
vary by board revision. See [ST's board manual (UM1472)](https://www.st.com/resource/en/user_manual/um1472-stm32f4-discovery-stmicroelectronics.pdf).
Configure the terminal to display LF as a new line; press Enter to submit commands.

After reset, check the HomeCore banner and prompt, then run `help`, `mem`,
`uptime`, `cat /dev/uptime`, `mkdir /tmp`, `touch /tmp/test`, `ls /tmp`, and
`reboot`. Uptime accuracy follows HSI oscillator tolerance. Polling RX can lose
characters during lengthy commands; send input interactively rather than
pasting large programs at full serial speed.

This port has been cross-compiled and its linked layout checked. The existing
LM3S6965EVB QEMU command regression passes; STM32 hardware validation is still
required. ST's pinned CMSIS device headers and license are in `external/stm32f4`.
