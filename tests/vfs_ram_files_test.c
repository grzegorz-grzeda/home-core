/* SPDX-License-Identifier: MIT */
#include "homecore/vfs/vfs.h"
#include "homecore/autoconf.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    assert(vfs_mkdir("/tmp") == 0);
    int fd = vfs_open("/tmp/file", O_CREAT | O_RDWR);
    assert(fd >= 0);
    assert(vfs_write(fd, "hello", 5) == 5);
    int reader = vfs_open("/tmp/file", O_RDONLY);
    char buffer[16] = {0};
    assert(vfs_read(reader, buffer, sizeof(buffer)) == 5);
    assert(strcmp(buffer, "hello") == 0);
    assert(vfs_read(reader, buffer, 1) == 0);
    assert(vfs_write(reader, "x", 1) == -1 && errno == EBADF);
    assert(vfs_unlink("/tmp/file") == -1 && errno == EBUSY);
    assert(vfs_rmdir("/tmp") == -1 && errno == ENOTEMPTY);
    assert(vfs_open("/tmp/file", O_CREAT | O_EXCL | O_RDWR) == -1 && errno == EEXIST);
    int append = vfs_open("/tmp/file", O_WRONLY | O_APPEND);
    assert(vfs_lseek(append, 0, SEEK_SET) == 0);
    assert(vfs_write(append, "!", 1) == 1);
    assert(vfs_read(append, buffer, 1) == -1 && errno == EBADF);
    assert(vfs_close(append) == 0);
    assert(vfs_lseek(fd, 8, SEEK_SET) == 8);
    assert(vfs_write(fd, "x", 1) == 1);
    assert(vfs_lseek(reader, 0, SEEK_SET) == 0);
    assert(vfs_read(reader, buffer, sizeof(buffer)) == 9);
    assert(memcmp(buffer, "hello!\0\0x", 9) == 0);
    assert(vfs_lseek(fd, CONFIG_HOMECORE_VFS_MAX_FILE_SIZE, SEEK_SET) ==
           CONFIG_HOMECORE_VFS_MAX_FILE_SIZE);
    assert(vfs_write(fd, "x", 1) == -1 && errno == EFBIG);
    assert(vfs_lseek(fd, -1, SEEK_SET) == -1);
    int truncate = vfs_open("/tmp/file", O_WRONLY | O_TRUNC);
    assert(truncate >= 0);
    assert(vfs_read(reader, buffer, 1) == 0); /* Previous read position beyond new EOF. */
    assert(vfs_close(truncate) == 0);
    assert(vfs_close(reader) == 0 && vfs_close(fd) == 0);
    assert(vfs_unlink("/tmp/file") == 0);
    assert(vfs_rmdir("/tmp") == 0);
    for (int i = 0; i < 2 * CONFIG_HOMECORE_VFS_MAX_DIRECTORIES; i++) {
        assert(vfs_mkdir("/reused") == 0);
        assert(vfs_rmdir("/reused") == 0);
    }
    for (int i = 0; i < 2 * CONFIG_HOMECORE_VFS_MAX_RAM_FILES; i++) {
        fd = vfs_open("/reused", O_CREAT | O_WRONLY);
        assert(fd >= 0 && vfs_close(fd) == 0 && vfs_unlink("/reused") == 0);
    }
    assert(vfs_open("/no-parent/file", O_CREAT | O_WRONLY) == -1 && errno == ENOENT);
    assert(vfs_open("/bad/", O_CREAT | O_WRONLY) == -1);
    assert(vfs_open("/bad", O_CREAT | O_RDONLY | O_TRUNC) == -1);
    assert(!vfs_find_node("/bad"));
    assert(vfs_rmdir("/") == -1 && errno == EBUSY);
    assert(vfs_rmdir("/dev") == -1 && errno == EBUSY);
    assert(vfs_unlink("/dev") == -1 && errno == EISDIR);
    static vfs_node_t device = {.name = "/dev/test"};
    vfs_register_node(&device);
    assert(vfs_unlink("/dev/test") == -1 && errno == EPERM);
    int descriptors[CONFIG_HOMECORE_VFS_MAX_OPEN_FILES];
    for (int i = 0; i < CONFIG_HOMECORE_VFS_MAX_OPEN_FILES; i++) {
        descriptors[i] = vfs_open("/dev/test", O_RDONLY);
        assert(descriptors[i] >= 0);
    }
    assert(vfs_open("/full", O_CREAT | O_WRONLY) == -1 && errno == EMFILE);
    assert(!vfs_find_node("/full"));
    for (int i = 0; i < CONFIG_HOMECORE_VFS_MAX_OPEN_FILES; i++) {
        assert(vfs_close(descriptors[i]) == 0);
    }
    for (int i = 0; i < CONFIG_HOMECORE_VFS_MAX_RAM_FILES; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/file%d", i);
        fd = vfs_open(path, O_CREAT | O_WRONLY);
        assert(fd >= 0 && vfs_close(fd) == 0);
    }
    assert(vfs_open("/overflow", O_CREAT | O_WRONLY) == -1 && errno == ENOSPC);
    puts("PASS: RAM files, append, truncate, sparse writes, limits, removal and slot reuse");
}
