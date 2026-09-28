/* SPDX-License-Identifier: MIT */
/* Link with -Wl,--wrap=calloc so the VFS's allocations can be made to fail. */
#include "homecore/vfs/vfs.h"
#include "homecore/autoconf.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

void *__real_calloc(size_t count, size_t size);
void *__wrap_calloc(size_t count, size_t size);

/* When nonzero, the calloc call with this 1-based number from now fails. */
static unsigned fail_countdown;

void *__wrap_calloc(size_t count, size_t size) {
    if (fail_countdown && --fail_countdown == 0) {
        return NULL;
    }
    return __real_calloc(count, size);
}

static void fail_allocation(unsigned number) {
    fail_countdown = number;
}

static int snapshot(vfs_node_t *node, char *buf, unsigned capacity) {
    (void)node;
    if (capacity < 2) {
        return -1;
    }
    memcpy(buf, "7\n", 2);
    return 2;
}

static vfs_node_t device = {.name = "/dev/snap", .ops = {.snapshot = snapshot}};

int main(void) {
    vfs_register_node(&device);
    char buffer[8] = {0};

    /* A failed directory allocation creates nothing and uses no capacity. */
    fail_allocation(1);
    assert(vfs_mkdir("/tmp") == -1 && errno == ENOMEM);
    assert(!vfs_find_node("/tmp") && errno == ENOENT);
    assert(vfs_mkdir("/tmp") == 0);

    /* O_CREAT: descriptor allocation fails first, so no file appears. */
    fail_allocation(1);
    assert(vfs_open("/tmp/file", O_CREAT | O_WRONLY) == -1 && errno == ENOMEM);
    assert(!vfs_find_node("/tmp/file"));

    /* O_CREAT: the file entry fails after the descriptor was allocated; the
     * descriptor is released, so the lowest descriptor stays free. */
    fail_allocation(2);
    assert(vfs_open("/tmp/file", O_CREAT | O_WRONLY) == -1 && errno == ENOMEM);
    assert(!vfs_find_node("/tmp/file"));
    int fd = vfs_open("/tmp/file", O_CREAT | O_WRONLY);
    assert(fd == 0);
    assert(vfs_write(fd, "abc", 3) == 3 && vfs_close(fd) == 0);

    /* O_TRUNC: a failed descriptor allocation must not truncate the file. */
    fail_allocation(1);
    assert(vfs_open("/tmp/file", O_WRONLY | O_TRUNC) == -1 && errno == ENOMEM);
    fd = vfs_open("/tmp/file", O_RDONLY);
    assert(fd == 0 && vfs_read(fd, buffer, sizeof(buffer)) == 3 && memcmp(buffer, "abc", 3) == 0);
    assert(vfs_close(fd) == 0);

    /* Snapshot descriptors carry their own buffer; failure leaves no descriptor. */
    fail_allocation(1);
    assert(vfs_open("/dev/snap", O_RDONLY) == -1 && errno == ENOMEM);
    fd = vfs_open("/dev/snap", O_RDONLY);
    assert(fd == 0 && vfs_read(fd, buffer, sizeof(buffer)) == 2 && memcmp(buffer, "7\n", 2) == 0);
    assert(vfs_close(fd) == 0);

    /* Failures did not consume capacity: the caps are reached exactly. */
    for (int i = 1; i < CONFIG_HOMECORE_VFS_MAX_DIRECTORIES; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/dir%d", i);
        assert(vfs_mkdir(path) == 0);
    }
    assert(vfs_mkdir("/full") == -1 && errno == ENOSPC);
    for (int i = 1; i < CONFIG_HOMECORE_VFS_MAX_RAM_FILES; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/file%d", i);
        fd = vfs_open(path, O_CREAT | O_WRONLY);
        assert(fd == 0 && vfs_close(fd) == 0);
    }
    assert(vfs_open("/overflow", O_CREAT | O_WRONLY) == -1 && errno == ENOSPC);
    puts("PASS: allocation failures leave namespace, descriptors, contents, and caps unchanged");
}
