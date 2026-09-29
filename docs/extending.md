# Extending HomeCore

This guide covers the two in-tree extension points: shell commands and VFS
device nodes. Read [architecture](architecture.md) first and follow the
[C coding standard](coding-standard.md). New boards, SoCs, and CPUs are covered
by [porting](porting.md).

## Shell commands

Commands are registered with `shell_register_command()` from
`include/homecore/shell/shell.h`:

```c
typedef int (*shell_command_handler_t)(shell_context_t *context, int argc, char **argv);
void shell_register_command(const char *name, const char *help, shell_command_handler_t handler);
```

Built-ins live in `src/subsystems/shell/builtin/`, grouped by area
(`builtin_files.c`, `builtin_session.c`, `builtin_system.c`, `builtin_basic.c`),
each with a private header. `shell_init()` in `src/subsystems/shell/shell.c`
registers them. To add one:

1. Implement the handler in the matching `builtin_*.c` file, or add a new
   `builtin_<area>.c`/`.h` pair and list the source in
   `src/subsystems/shell/builtin/CMakeLists.txt`.
2. Register it in `shell_init()` with a one-line help text that shows the usage,
   for example `"Remove files: rm path..."`.
3. Document it in [shell](shell.md) and add a command check to
   `tests/qemu_files_test.py` when its behavior is observable on the console.

Handler contract:

- `argv[0]` is the command name; `argv[argc]` is `NULL`. Tokens are split on
  spaces and tabs only. There is no quoting, escaping, globbing, or redirection.
  At most `CONFIG_HOMECORE_SHELL_MAX_ARGS` tokens are accepted.
- Tokens point into the shell's input buffer and are valid only during the call.
- The return value becomes the session's `last_status`. Return 0 on success and
  a nonzero status on failure; the shell uses 2 for excessive arguments and 127
  for unknown commands.
- The dispatcher makes `context` the current session for the duration of the
  call, so libc path operations (`fopen`, `open`) resolve against its working
  directory. For explicit VFS calls, resolve paths first with
  `vfs_resolve_path(context->cwd, path, resolved)`.
- Handlers run in the foreground loop, not in interrupt context, and may block.
  Print errors to stdout, as the existing built-ins do.

Registration stores the `name` and `help` pointers without copying them. Pass
string literals or other storage that outlives the shell. Each registration
allocates a list entry from the heap. On allocation failure or a `NULL`
argument, the command is silently not registered. New entries are prepended, so
`help` lists commands in reverse registration order, and a later registration
with the same name shadows the earlier one.

`tests/session_test.c` links `shell.c` and all built-in sources except
`builtin_basic.c`, which it stubs. A new built-in source file referenced from
`shell_init()` must be added to that test's source list in both
[development](development.md#validation) and `scripts/check_quality.py`, or
stubbed in the test.

## VFS device nodes

A device is a statically allocated `vfs_node_t` (`include/homecore/vfs/vfs.h`)
registered with `vfs_register_node()`. See `src/soc/ti/lm3s6965/soc.c` for
stream devices (UARTs) and `src/kernel/uptime.c` for a snapshot device.

```c
static vfs_node_t example_node = {
    .name = "/dev/example",
    .ops = {.read = example_read, .write = example_write},
    .driver_data = &example_state,
};

vfs_register_node(&example_node);
```

Registration rules:

- `name` is the node's canonical absolute path, such as `/dev/uart0`. It is
  matched by exact string comparison after path resolution. Place devices under
  the existing `/dev` directory; registration does not check that the parent
  exists or create it.
- A second registration with an existing name is silently ignored.
- The node and its name are linked into the VFS list by pointer. They must remain
  valid for the rest of the program; there is no unregister operation.
- Register during startup: hardware devices through a driver from `dt_init()`
  (see [device drivers](#device-drivers)), kernel devices such as `/dev/uptime`
  in `k_init()`. Do not call `vfs_init()` after registration; see
  [architecture](architecture.md#startup).

### Stream devices

Set `read`, `write`, and optionally `open`, `close`, `ioctl`, and `lseek`.
Descriptor-level calls validate the descriptor and forward to the node
operation. A missing operation makes the call fail with -1. Operations receive
the node, so drivers keep per-device state in `driver_data`. The VFS does not
keep per-descriptor state for stream devices, and all descriptors open on one
node share the driver's state.

- `read`/`write` return the number of bytes transferred, or -1 with `errno`
  set. Handle `len == 0` and a `NULL` buffer explicitly, as the UART drivers do.
  Returning 0 from `read` signals EOF to libc; a blocking console should
  instead wait for data.
- Lengths arrive as `unsigned`, but results are `int`. Reject lengths above
  `INT_MAX` (for example with `EOVERFLOW`) rather than returning a wrapped count.
- If `open` returns a negative value, `vfs_open()` releases the descriptor and
  returns that value.
- The UART nodes back stdin, stdout, and stderr. `k_init()` opens `/dev/uart0`
  for descriptors 0, 1, and 2, so its operations must work before any other
  output path is available.

### Snapshot devices

For small read-only status files, set only `snapshot`:

```c
int (*snapshot)(vfs_node_t *node, char *buf, unsigned capacity);
```

`vfs_open()` calls it once per open with `VFS_SNAPSHOT_CAPACITY` (32) bytes of
per-descriptor storage. Return the byte length written, or -1 if the content
does not fit. `vfs_open()` then fails with `EIO`. The VFS handles partial reads,
EOF, and seeking within the captured content. Writes fail with `EBADF`. Opening
for writing, or with `O_TRUNC`, `O_APPEND`, or `O_CREAT`, fails with `EACCES`.
Snapshots run in the caller's foreground context; mask interrupts only around
reads of state shared with interrupt handlers, as `k_uptime_ms()` does.

### Return conventions

VFS descriptor functions return -1 for an out-of-range descriptor and -2 for a
descriptor that is not open, without setting `errno`. Other VFS failures return
-1 with `errno` set. libc hooks in `src/kernel/syscalls.c` pass these results
through, so new callers should test for a negative result rather than
comparing with -1.

### Testing

Portable VFS logic is covered by host tests in `tests/` that compile
`src/subsystems/vfs/vfs.c` directly. Add or extend a host test for new node
behavior, and use `tests/uart_contract_test.c` as a model for driver callbacks
with mocked registers. Commands are listed in
[development](development.md#validation).

## Device drivers

Hardware devices come from the [device description](development.md#device-description).
A driver serves one compatible and lives in `src/drivers/<class>/`, for example
`src/drivers/serial/stm32_usart.c`. The serial drivers are the reference.

1. **Binding** `<compatible>.yaml`: `compatible`, `driver` (the C prefix),
   `sources`, `console: true` if it can be `chosen.console`, and `properties`
   with types and the configuration `field` each one fills.
2. **Header** `<driver>.h`: `<driver>_config_t` (constants from the
   description, placed in flash), `<driver>_t` (per-instance state whose first
   field is `const <driver>_config_t *config`), `void <driver>_init(<driver>_t *)`,
   and `extern const console_ops_t <driver>_console_ops` for console-capable
   drivers.
3. **Source**: `<driver>_init()` programs the hardware, enables its interrupt
   with `arch_irq_enable()` if it uses one, and registers a VFS node named by
   the `devpath` property. With `isr: true` in the binding and an `irq`
   property, the generated `dt_irq_dispatch()` calls `<driver>_isr(<driver>_t *)`
   for that interrupt. Keep it short and bounded; `rx_ring.h` is the
   interrupt-to-reader buffer the serial drivers share. It runs from `dt_init()`, after
   `board_init()` has enabled clocks and configured pins, and must not print.
4. **SoC description**: add the instances to `soc.yaml` with `status: disabled`;
   boards enable them in `board.yaml`.

A driver is compiled only when a board enables one of its devices. Access
registers through the SoC's CMSIS types (`soc_cmsis.h`); host tests substitute
a stand-in header from `tests/fakes/`, as `tests/uart_contract_test.c` does.
Add generator cases for new property rules to `tests/devicetree_generate_test.py`.

### GPIO ports and LEDs

A GPIO port driver sets `gpio: true` and exports
`const gpio_ops_t <driver>_gpio_ops` (`include/homecore/drivers/gpio.h`):
configure, set, and get a pin. Its init function enables the port clock; the
ports are SoC devices, like UARTs. A driver that uses a pin declares a `gpio`
property, which fills a `gpio_port_t`, and a `pin`; the generator initializes
the port first. `src/drivers/gpio/stm32f4_gpio.c` is the reference.

An LED driver sets `led: true`, exports `const led_ops_t <driver>_led_ops`
(`include/homecore/drivers/led.h`), and calls `led_register_node()` from its
init function for the `/dev/ledN` file; `src/drivers/led/led.c` implements the
file, numbering, and the `led_*()` API. `homecore,gpio-led` drives a GPIO pin
and `homecore,console-led` prints changes for emulators without LEDs. A board
describes an emulator variant as disabled console LEDs with the same `devname`
and swaps them in from an overlay, as `src/board/stm32vldiscovery/qemu.yaml`
does.

### Block devices

A block driver sets `block: true` in its binding and exports
`const block_ops_t <driver>_block_ops` (`include/homecore/drivers/block.h`):
sector read, write, sync, and sector count, with 512-byte sectors. It
registers no VFS node; a board's `mounts` entry attaches a filesystem, and the
generator builds the `block_device_t`. The ramdisk
(`src/drivers/block/ramdisk.c`, compatible `homecore,ramdisk`) is the
reference and shows the `size` property with a generated `buffer`.

## Mounted filesystems

A filesystem implements every operation of `vfs_fs_ops_t`
(`include/homecore/vfs/vfs.h`) and attaches itself with `vfs_mount()`. The VFS
resolves `.` and `..`, checks access modes, allocates descriptors, and passes
paths relative to the mount point (`/` is the mount point itself); the
filesystem handles `O_CREAT`, `O_EXCL`, `O_TRUNC`, and `O_APPEND` and returns
the same errno values as RAM files. Paths below a mount point have no
`vfs_node_t`, so code that needs a type or size uses `vfs_stat()` or
`vfs_fstat()` rather than `vfs_find_node()`. The FAT glue in
`src/subsystems/fs/fat/fat.c` and the littlefs glue in
`src/subsystems/fs/littlefs/littlefs.c` are the references; add the
filesystem's type and mount function to `FILESYSTEMS` in
`scripts/devicetree_generate.py` so boards can name it, and compile its
library in `src/subsystems/fs/CMakeLists.txt` only when
`HOMECORE_DT_FILESYSTEMS` names it. The API reference's
[Files and storage](https://grzegorz-grzeda.github.io/home-core/storage.html#storage_extending)
page lists the full contract.
