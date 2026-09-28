/* SPDX-License-Identifier: MIT */
#include "homecore/kernel/kernel.h"
#include "homecore/vfs/vfs.h"
#include "homecore/board/board.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/* Unused newlib hooks are removed by section GC in the host test link. */
#include "../src/kernel/syscalls.c"
#include "../src/kernel/kernel.c"

static unsigned io_calls;
static unsigned last_length;
static int open_calls;
static int failure_at;
static int unexpected_fd;
static unsigned closed;
static jmp_buf panic_env;

int vfs_read(int fd, void *buf, unsigned len) {
    (void)fd;
    (void)buf;
    ++io_calls;
    last_length = len;
    return 0;
}
int vfs_write(int fd, const void *buf, unsigned len) {
    (void)fd;
    (void)buf;
    ++io_calls;
    last_length = len;
    return 0;
}
void k_uptime_init(void) {
}
int vfs_open(const char *name, int flags) {
    assert(strcmp(name, "/dev/uart0") == 0);
    assert(flags == (open_calls == 0 ? O_RDONLY : O_WRONLY));
    int current = open_calls++;
    if (current == failure_at) {
        return unexpected_fd;
    }
    return current;
}
int vfs_close(int fd) {
    assert(fd >= 0 && fd < 8);
    closed |= 1U << (unsigned)fd;
    return 0;
}
void board_panic(const char *message) {
    assert(message && *message);
    longjmp(panic_env, 1);
}

int main(void) {
    int result = _read(0, NULL, -1);
    assert(result == -1 && errno == EINVAL && io_calls == 0);
    result = _write(1, NULL, INT_MIN);
    assert(result == -1 && errno == EINVAL && io_calls == 0);
    result = _read(0, NULL, 0);
    assert(result == 0 && last_length == 0 && io_calls == 1);
    result = _write(1, NULL, INT_MAX);
    assert(result == 0 && last_length == INT_MAX && io_calls == 2);

    failure_at = -1;
    k_init();
    assert(open_calls == 3 && closed == 0);
    for (failure_at = 0; failure_at < 3; ++failure_at) {
        open_calls = 0;
        closed = 0;
        unexpected_fd = -1;
        if (setjmp(panic_env) == 0) {
            k_init();
            assert(!"console failure must panic");
        }
        assert(open_calls == failure_at + 1);
        assert(closed == (1U << (unsigned)failure_at) - 1U);
    }
    open_calls = 0;
    closed = 0;
    failure_at = 1;
    unexpected_fd = 5;
    if (setjmp(panic_env) == 0) {
        k_init();
        assert(!"unexpected descriptor must panic");
    }
    assert(closed == ((1U << 0) | (1U << 5)) && open_calls == 2);
    puts("PASS: signed I/O lengths, console descriptors, failure cleanup");
    return 0;
}
