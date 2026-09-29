/* SPDX-License-Identifier: MIT */
/* FAT volumes on ramdisks, used through the VFS. */
#include "homecore/autoconf.h"
#include "homecore/fs/fat.h"
#include "homecore/vfs/vfs.h"
#include "ramdisk.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static uint8_t small_data[32 * 1024];
static const ramdisk_config_t small_config = {.data = small_data, .size = sizeof(small_data)};
static ramdisk_t small_disk = {.config = &small_config};
static const block_device_t small_block = {.ops = &ramdisk_block_ops, .device = &small_disk};

static uint8_t disk_data[64 * 1024];
static const ramdisk_config_t disk_config = {.data = disk_data, .size = sizeof(disk_data)};
static ramdisk_t disk = {.config = &disk_config};
static const block_device_t block = {.ops = &ramdisk_block_ops, .device = &disk};

static char entries[256];
static int collect(const char *name, bool directory, void *context) {
    (void)context;
    strcat(entries, name);
    strcat(entries, directory ? "/\n" : "\n");
    return 0;
}

static void list(const char *path) {
    entries[0] = '\0';
    assert(vfs_list(path, collect, NULL) == 0);
}

static void check_mounts(void) {
    assert(fs_fat_mount(&block, "/ram", false) == -1 && errno == ENODEV);
    assert(!vfs_find_node("/ram"));
    assert(fs_fat_mount(&small_block, "/small", true) == -1 && errno == EINVAL);
    assert(fs_fat_mount(&block, "/missing/ram", true) == -1 && errno == ENOENT);
    assert(fs_fat_mount(&block, "/ram", true) == 0);
    assert(vfs_mount("/ram", NULL, NULL) == -1 && errno == EINVAL);

    vfs_stat_t stat;
    assert(vfs_stat("/ram", &stat) == 0 && stat.is_directory && !stat.is_regular);
    assert(vfs_mkdir("/ram") == -1 && errno == EEXIST);
    assert(vfs_rmdir("/ram") == -1 && errno == EBUSY);
    assert(vfs_unlink("/ram") == -1 && errno == EISDIR);
    assert(vfs_open("/ram", O_RDONLY) == -1 && errno == EISDIR);
    list("/");
    assert(strcmp(entries, "ram/\ndev/\n") == 0); /* Newest first. */
    list("/ram");
    assert(entries[0] == '\0');
}

static void check_files(void) {
    int fd = vfs_open("/ram/hello.txt", O_CREAT | O_RDWR);
    assert(fd >= 0);
    assert(!vfs_fd_node(fd) && errno == ENOTSUP);
    assert(vfs_ioctl(fd, 0, NULL) == -1 && errno == ENOTTY);
    assert(vfs_write(fd, "hello", 5) == 5);
    assert(vfs_lseek(fd, 0, SEEK_SET) == 0);
    char buffer[16] = {0};
    assert(vfs_read(fd, buffer, sizeof(buffer)) == 5 && strcmp(buffer, "hello") == 0);
    assert(vfs_read(fd, buffer, 1) == 0);
    assert(vfs_lseek(fd, 1, SEEK_END) == -1 && errno == EINVAL);
    assert(vfs_lseek(fd, -1, SEEK_SET) == -1 && errno == EINVAL);
    assert(vfs_lseek(fd, -2, SEEK_END) == 3);
    vfs_stat_t stat;
    assert(vfs_fstat(fd, &stat) == 0 && stat.is_regular && stat.size == 5);
    assert(vfs_unlink("/ram/hello.txt") == -1 && errno == EBUSY);
    assert(vfs_close(fd) == 0);
    assert(vfs_fstat(fd, &stat) == -1 && errno == EBADF);

    assert(vfs_stat("/ram/hello.txt", &stat) == 0 && stat.is_regular && stat.size == 5);
    assert(vfs_open("/ram/hello.txt", O_CREAT | O_EXCL | O_WRONLY) == -1 && errno == EEXIST);
    assert(vfs_open("/ram/absent", O_RDONLY) == -1 && errno == ENOENT);
    assert(vfs_open("/ram/hello.txt/x", O_CREAT | O_WRONLY) == -1 && errno == ENOTDIR);

    int append = vfs_open("/ram/hello.txt", O_WRONLY | O_APPEND);
    assert(append >= 0);
    assert(vfs_lseek(append, 0, SEEK_SET) == 0);
    assert(vfs_write(append, "!", 1) == 1);
    assert(vfs_read(append, buffer, 1) == -1 && errno == EBADF);
    assert(vfs_close(append) == 0);
    int reader = vfs_open("/ram/hello.txt", O_RDONLY);
    memset(buffer, 0, sizeof(buffer));
    assert(vfs_read(reader, buffer, sizeof(buffer)) == 6 && strcmp(buffer, "hello!") == 0);
    assert(vfs_write(reader, "x", 1) == -1 && errno == EBADF);
    assert(vfs_close(reader) == 0);

    int truncate = vfs_open("/ram/hello.txt", O_WRONLY | O_TRUNC);
    assert(truncate >= 0 && vfs_close(truncate) == 0);
    assert(vfs_stat("/ram/hello.txt", &stat) == 0 && stat.size == 0);
    assert(vfs_open("/ram/hello.txt", O_RDONLY | O_TRUNC) == -1 && errno == EACCES);
#if CONFIG_HOMECORE_FS_FAT_LFN
    int named = vfs_open("/ram/A longer file name.text", O_CREAT | O_WRONLY);
    assert(named >= 0 && vfs_close(named) == 0);
    assert(vfs_unlink("/ram/a LONGER file NAME.text") == 0);
#endif
    assert(vfs_unlink("/ram/hello.txt") == 0);
    assert(vfs_stat("/ram/hello.txt", &stat) == -1 && errno == ENOENT);
}

static void check_directories(void) {
    assert(vfs_mkdir("/ram/dir") == 0);
    assert(vfs_mkdir("/ram/dir/sub/") == 0);
    assert(vfs_mkdir("/ram/dir") == -1 && errno == EEXIST);
    assert(vfs_mkdir("/ram/none/sub") == -1 && errno == ENOENT);
    int fd = vfs_open("/ram/dir/file", O_CREAT | O_WRONLY);
    assert(fd >= 0 && vfs_close(fd) == 0);
    list("/ram/dir/sub/..");
    assert(strcmp(entries, "sub/\nfile\n") == 0);
    assert(vfs_open("/ram/dir/sub", O_RDONLY) == -1 && errno == EISDIR);
    assert(vfs_rmdir("/ram/dir") == -1 && errno == ENOTEMPTY);
    assert(vfs_unlink("/ram/dir/sub") == -1 && errno == EISDIR);
    assert(vfs_rmdir("/ram/dir/file") == -1 && errno == ENOTDIR);
    assert(vfs_mkdir("/ram/dir/file/x") == -1 && errno == ENOTDIR);
    assert(vfs_list("/ram/dir/file", collect, NULL) == -1 && errno == ENOTDIR);
    char resolved[VFS_PATH_CAPACITY];
    assert(vfs_resolve_path("/ram/dir/sub", "../../..", resolved) == 0);
    assert(strcmp(resolved, "/") == 0);
    assert(vfs_unlink("/ram/dir/file") == 0);
    assert(vfs_rmdir("/ram/dir/sub") == 0);
    assert(vfs_rmdir("/ram/dir") == 0);
    list("/ram");
    assert(entries[0] == '\0');
}

static void check_full_disk(void) {
    static char chunk[4096];
    int fd = vfs_open("/ram/big", O_CREAT | O_WRONLY);
    assert(fd >= 0);
    unsigned total = 0;
    int written;
    while ((written = vfs_write(fd, chunk, sizeof(chunk))) > 0) {
        total += (unsigned)written;
    }
    assert(written == -1 && errno == ENOSPC);
    assert(total > 32U * 1024U && total < sizeof(disk_data));
    assert(vfs_close(fd) == 0);
    assert(vfs_unlink("/ram/big") == 0);
}

static int fake_stat(void *fs, const char *path, vfs_stat_t *stat) {
    (void)fs;
    (void)path;
    *stat = (vfs_stat_t){.is_directory = true};
    return 0;
}

static void check_mount_limits(void) {
    /* Only the checks before any filesystem operation run for these. */
    static const vfs_fs_ops_t fake = {.stat = fake_stat};
    assert(vfs_mount("/ram/inner", &fake, NULL) == -1 && errno == EBUSY);
    assert(vfs_mount("/dev", &fake, NULL) == -1 && errno == EEXIST);
    for (unsigned i = 1; i < CONFIG_HOMECORE_VFS_MAX_MOUNTS; i++) {
        char path[16];
        snprintf(path, sizeof(path), "/m%u", i);
        assert(vfs_mount(path, &fake, NULL) == 0);
    }
    assert(vfs_mount("/one_more", &fake, NULL) == -1 && errno == ENOSPC);
}

int main(void) {
    check_mounts();
    check_files();
    check_directories();
    check_full_disk();
    check_mount_limits();
    puts("fat: ok");
    return 0;
}
