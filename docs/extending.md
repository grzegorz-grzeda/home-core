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
   for example `"Remove RAM files: rm path..."`.
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
- Register during startup: SoC devices in `soc_init()`, kernel devices in
  `k_init()`. Registration must not depend on the console, because `soc_init()`
  runs before `board_init()`. Do not call `vfs_init()` after registration; see
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
