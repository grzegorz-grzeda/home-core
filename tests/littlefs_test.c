/* SPDX-License-Identifier: MIT */
/* littlefs volumes on ramdisks, used through the VFS, and power-loss
 * resilience of the underlying configuration. */
#include "homecore/autoconf.h"
#include "homecore/fs/littlefs.h"
#include "homecore/vfs/vfs.h"
#include "littlefs_private.h"
#include "ramdisk.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#define DISK_SIZE (32U * 1024U)

static uint8_t tiny_data[3U * BLOCK_SECTOR_SIZE];
static const ramdisk_config_t tiny_config = {.data = tiny_data, .size = sizeof(tiny_data)};
static ramdisk_t tiny_disk = {.config = &tiny_config};
static const block_device_t tiny_block = {.ops = &ramdisk_block_ops, .device = &tiny_disk};

static uint8_t disk_data[DISK_SIZE];
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
    assert(fs_littlefs_mount(NULL, "/lfs", true) == -1 && errno == EINVAL);
    assert(fs_littlefs_mount(&tiny_block, "/lfs", true) == -1 && errno == EINVAL);
    assert(fs_littlefs_mount(&block, "/lfs", false) == -1 && errno == ENODEV);
    assert(!vfs_find_node("/lfs"));
    assert(fs_littlefs_mount(&block, "/missing/lfs", true) == -1 && errno == ENOENT);
    assert(fs_littlefs_mount(&block, "/lfs", true) == 0);
    assert(fs_littlefs_mount(&block, "/lfs", true) == -1 && errno == EBUSY);

    vfs_stat_t stat;
    assert(vfs_stat("/lfs", &stat) == 0 && stat.is_directory);
    assert(vfs_rmdir("/lfs") == -1 && errno == EBUSY);
    list("/lfs");
    assert(entries[0] == '\0'); /* "." and ".." are not listed. */
}

static void check_files(void) {
    int fd = vfs_open("/lfs/hello.txt", O_CREAT | O_RDWR);
    assert(fd >= 0);
    assert(!vfs_fd_node(fd) && errno == ENOTSUP);
    assert(vfs_write(fd, "hello", 5) == 5);
    assert(vfs_lseek(fd, 0, SEEK_SET) == 0);
    char buffer[16] = {0};
    assert(vfs_read(fd, buffer, sizeof(buffer)) == 5 && strcmp(buffer, "hello") == 0);
    assert(vfs_read(fd, buffer, 1) == 0);
    assert(vfs_lseek(fd, -1, SEEK_SET) == -1 && errno == EINVAL);
    assert(vfs_lseek(fd, 0, 42) == -1 && errno == EINVAL);
    /* Writing past the end fills the gap with zeros, as for RAM files. */
    assert(vfs_lseek(fd, 8, SEEK_SET) == 8);
    assert(vfs_write(fd, "x", 1) == 1);
    assert(vfs_lseek(fd, 0, SEEK_SET) == 0);
    memset(buffer, 0xff, sizeof(buffer));
    assert(vfs_read(fd, buffer, sizeof(buffer)) == 9);
    assert(memcmp(buffer, "hello\0\0\0x", 9) == 0);
    vfs_stat_t stat;
    assert(vfs_fstat(fd, &stat) == 0 && stat.is_regular && stat.size == 9);
    assert(vfs_unlink("/lfs/hello.txt") == -1 && errno == EBUSY);
    assert(vfs_close(fd) == 0);

    assert(vfs_stat("/lfs/hello.txt", &stat) == 0 && stat.is_regular && stat.size == 9);
    assert(vfs_open("/lfs/hello.txt", O_CREAT | O_EXCL | O_WRONLY) == -1 && errno == EEXIST);
    assert(vfs_open("/lfs/absent", O_RDONLY) == -1 && errno == ENOENT);
    assert(vfs_open("/lfs/hello.txt/x", O_CREAT | O_WRONLY) == -1 && errno == ENOTDIR);

    int append = vfs_open("/lfs/hello.txt", O_WRONLY | O_APPEND);
    assert(append >= 0);
    assert(vfs_lseek(append, 0, SEEK_SET) == 0);
    assert(vfs_write(append, "!", 1) == 1);
    assert(vfs_read(append, buffer, 1) == -1 && errno == EBADF);
    assert(vfs_close(append) == 0);
    assert(vfs_stat("/lfs/hello.txt", &stat) == 0 && stat.size == 10);

    int truncate = vfs_open("/lfs/hello.txt", O_WRONLY | O_TRUNC);
    assert(truncate >= 0 && vfs_close(truncate) == 0);
    assert(vfs_stat("/lfs/hello.txt", &stat) == 0 && stat.size == 0);
    /* Names are case-sensitive, unlike FAT. */
    assert(vfs_stat("/lfs/HELLO.TXT", &stat) == -1 && errno == ENOENT);
    assert(vfs_unlink("/lfs/hello.txt") == 0);
    assert(vfs_stat("/lfs/hello.txt", &stat) == -1 && errno == ENOENT);
}

static void check_directories(void) {
    assert(vfs_mkdir("/lfs/dir") == 0);
    assert(vfs_mkdir("/lfs/dir/sub/") == 0);
    assert(vfs_mkdir("/lfs/dir") == -1 && errno == EEXIST);
    assert(vfs_mkdir("/lfs/none/sub") == -1 && errno == ENOENT);
    int fd = vfs_open("/lfs/dir/file", O_CREAT | O_WRONLY);
    assert(fd >= 0 && vfs_close(fd) == 0);
    list("/lfs/dir/sub/..");
    assert(strstr(entries, "sub/\n") && strstr(entries, "file\n") && strlen(entries) == 10);
    assert(vfs_open("/lfs/dir/sub", O_RDONLY) == -1 && errno == EISDIR);
    assert(vfs_rmdir("/lfs/dir") == -1 && errno == ENOTEMPTY);
    assert(vfs_unlink("/lfs/dir/sub") == -1 && errno == EISDIR);
    assert(vfs_rmdir("/lfs/dir/file") == -1 && errno == ENOTDIR);
    assert(vfs_mkdir("/lfs/dir/file/x") == -1 && errno == ENOTDIR);
    assert(vfs_list("/lfs/dir/file", collect, NULL) == -1 && errno == ENOTDIR);
    char resolved[VFS_PATH_CAPACITY];
    assert(vfs_resolve_path("/lfs/dir/sub", "../file", resolved) == 0);
    assert(strcmp(resolved, "/lfs/dir/file") == 0);
    assert(vfs_unlink("/lfs/dir/file") == 0);
    assert(vfs_rmdir("/lfs/dir/sub") == 0);
    assert(vfs_rmdir("/lfs/dir") == 0);
    list("/lfs");
    assert(entries[0] == '\0');
}

static void check_full_disk(void) {
    static char chunk[1024];
    int fd = vfs_open("/lfs/big", O_CREAT | O_WRONLY);
    assert(fd >= 0);
    unsigned total = 0;
    int written;
    while ((written = vfs_write(fd, chunk, sizeof(chunk))) > 0) {
        total += (unsigned)written;
    }
    assert(written == -1 && errno == ENOSPC);
    assert(total > DISK_SIZE / 4U && total < DISK_SIZE);
    (void)vfs_close(fd); /* May also report ENOSPC for the unwritten tail. */
    assert(vfs_unlink("/lfs/big") == 0);
    fd = vfs_open("/lfs/after", O_CREAT | O_WRONLY);
    assert(fd >= 0 && vfs_write(fd, "ok", 2) == 2 && vfs_close(fd) == 0);
}

/* Data written through the VFS is on the device: a second, independent
 * littlefs instance reads it back. Read-only, so the mounted volume's view
 * stays valid. */
static void check_persistence(void) {
    struct lfs_config config;
    littlefs_config_init(&config, &block);
    lfs_t lfs;
    assert(lfs_mount(&lfs, &config) == 0);
    lfs_file_t file;
    assert(lfs_file_open(&lfs, &file, "/after", LFS_O_RDONLY) == 0);
    char buffer[4] = {0};
    assert(lfs_file_read(&lfs, &file, buffer, sizeof(buffer)) == 2);
    assert(strcmp(buffer, "ok") == 0);
    assert(lfs_file_close(&lfs, &file) == 0);
    assert(lfs_unmount(&lfs) == 0);
}

/* A block device that loses power after a set number of sector writes: later
 * writes fail without changing the disk, as if the board had reset. */
static unsigned writes_left;
static bool power_lost;

static int cut_read(void *device, uint32_t sector, void *buf, uint32_t count) {
    return ramdisk_block_ops.read(device, sector, buf, count);
}

static int cut_write(void *device, uint32_t sector, const void *buf, uint32_t count) {
    if (writes_left == 0) {
        power_lost = true;
        errno = EIO;
        return -1;
    }
    writes_left--;
    return ramdisk_block_ops.write(device, sector, buf, count);
}

static int cut_sync(void *device) {
    return ramdisk_block_ops.sync(device);
}

static uint32_t cut_sector_count(void *device) {
    return ramdisk_block_ops.sector_count(device);
}

static const block_ops_t cut_ops = {
    .read = cut_read,
    .write = cut_write,
    .sync = cut_sync,
    .sector_count = cut_sector_count,
};

static uint8_t cut_data[16U * 1024U];
static uint8_t cut_image[sizeof(cut_data)];
static const ramdisk_config_t cut_config = {.data = cut_data, .size = sizeof(cut_data)};
static ramdisk_t cut_disk = {.config = &cut_config};
static const block_device_t cut_block = {.ops = &cut_ops, .device = &cut_disk};

static char old_text[1500];
static char new_text[1500];

static void write_file(lfs_t *lfs, const char *text, size_t size) {
    lfs_file_t file;
    if (lfs_file_open(lfs, &file, "/data", LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC) < 0) {
        return;
    }
    (void)lfs_file_write(lfs, &file, text, (lfs_size_t)size);
    (void)lfs_file_close(lfs, &file);
}

/* After a reset at any write, the volume mounts and the file holds either
 * the old or the new contents, never a mix. */
static void check_power_loss(void) {
    memset(old_text, 'o', sizeof(old_text));
    memset(new_text, 'n', sizeof(new_text));
    struct lfs_config config;
    littlefs_config_init(&config, &cut_block);
    lfs_t lfs;
    writes_left = UINT32_MAX;
    assert(lfs_format(&lfs, &config) == 0 && lfs_mount(&lfs, &config) == 0);
    write_file(&lfs, old_text, sizeof(old_text));
    assert(lfs_unmount(&lfs) == 0);
    memcpy(cut_image, cut_data, sizeof(cut_image));

    unsigned cuts = 0;
    for (unsigned limit = 0;; limit++) {
        memcpy(cut_data, cut_image, sizeof(cut_data));
        power_lost = false;
        writes_left = limit;
        assert(lfs_mount(&lfs, &config) == 0);
        write_file(&lfs, new_text, sizeof(new_text));
        (void)lfs_unmount(&lfs);
        bool completed = !power_lost;

        writes_left = UINT32_MAX;
        assert(lfs_mount(&lfs, &config) == 0);
        lfs_file_t file;
        assert(lfs_file_open(&lfs, &file, "/data", LFS_O_RDONLY) == 0);
        static char contents[sizeof(old_text) + 1];
        lfs_ssize_t length = lfs_file_read(&lfs, &file, contents, sizeof(contents));
        assert(lfs_file_close(&lfs, &file) == 0);
        assert(lfs_unmount(&lfs) == 0);
        assert(length == (lfs_ssize_t)sizeof(old_text));
        bool is_old = memcmp(contents, old_text, sizeof(old_text)) == 0;
        bool is_new = memcmp(contents, new_text, sizeof(new_text)) == 0;
        assert(is_old || is_new);
        if (completed) {
            assert(is_new);
            break;
        }
        cuts++;
    }
    assert(cuts > 0);
    printf("littlefs: survived %u power cuts\n", cuts);
}

int main(void) {
    check_mounts();
    check_files();
    check_directories();
    check_full_disk();
    check_persistence();
    check_power_loss();
    puts("littlefs: ok");
    return 0;
}
