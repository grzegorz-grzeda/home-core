/**
 * MIT License
 *
 * Copyright (c) 2026 Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*---------------------------------------------------------------------------*/
#include "homecore/kernel/kernel.h"
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
#include <malloc.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#include "homecore/board/board.h"
#include "homecore/vfs/vfs.h"
#include "homecore/session/session.h"
/*---------------------------------------------------------------------------*/
extern uint8_t _heap_start;
extern uint8_t _heap_end;
/*---------------------------------------------------------------------------*/
static uint8_t *heap_current = &_heap_start;
/*---------------------------------------------------------------------------*/
void *_sbrk(ptrdiff_t incr) {
    uintptr_t current = (uintptr_t)heap_current;
    uintptr_t start = (uintptr_t)&_heap_start, end = (uintptr_t)&_heap_end;
    size_t amount = incr < 0 ? (size_t)(-(incr + 1)) + 1 : (size_t)incr;
    if ((incr < 0 && amount > current - start) || (incr >= 0 && amount > end - current)) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_current = (uint8_t *)(incr < 0 ? current - amount : current + amount);
    return (void *)current;
}

void k_heap_stats(k_heap_stats_t *stats) {
    struct mallinfo info = mallinfo();
    stats->total = (uintptr_t)&_heap_end - (uintptr_t)&_heap_start;
    stats->allocated = (size_t)info.uordblks;
    stats->reusable = (size_t)info.fordblks;
    stats->unclaimed = (uintptr_t)&_heap_end - (uintptr_t)heap_current;
}
/*---------------------------------------------------------------------------*/
int _open(const char *name, int flags, ...) {
    char path[VFS_PATH_CAPACITY];
    if (vfs_resolve_path(session_current()->cwd, name, path) < 0) {
        return -1;
    }
    return vfs_open(path, flags);
}
/*---------------------------------------------------------------------------*/
int _write(int fd, const char *buf, int len) {
    if (len < 0) {
        errno = EINVAL;
        return -1;
    }
    return vfs_write(fd, buf, (unsigned)len);
}
/*---------------------------------------------------------------------------*/
int _read(int fd, char *buf, int len) {
    if (len < 0) {
        errno = EINVAL;
        return -1;
    }
    return vfs_read(fd, buf, (unsigned)len);
}
/*---------------------------------------------------------------------------*/
int _close(int fd) {
    return vfs_close(fd);
}
/*---------------------------------------------------------------------------*/
int _fstat(int fd, struct stat *st) {
    vfs_node_t *node = vfs_fd_node(fd);
    if (!node) {
        return -1;
    }
    if (!st) {
        errno = EFAULT;
        return -1;
    }
    memset(st, 0, sizeof(*st));
    st->st_mode = node->is_regular ? S_IFREG : S_IFCHR;
    st->st_size = node->size;
    return 0;
}
/*---------------------------------------------------------------------------*/
int _isatty(int fd) {
    vfs_node_t *node = vfs_fd_node(fd);
    if (!node) {
        return 0;
    }
    if (node->is_regular || node->ops.snapshot) {
        errno = ENOTTY;
        return 0;
    }
    return 1;
}
/*---------------------------------------------------------------------------*/
int _lseek(int fd, int ptr, int dir) {
    return vfs_lseek(fd, ptr, dir);
}
/*---------------------------------------------------------------------------*/
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;

    errno = EINVAL;
    return -1;
}
/*---------------------------------------------------------------------------*/
int _getpid(void) {
    return 1;
}
/*---------------------------------------------------------------------------*/
void _exit(int status) {
    (void)status;
    while (1) {
        // Infinite loop to halt the program
    }
}