// SPDX-License-Identifier: MIT
/* littlefs volumes in the VFS: translates vfs_fs_ops_t calls into littlefs
 * calls and littlefs results into errno values. */
#include "homecore/fs/littlefs.h"
#include "homecore/vfs/vfs.h"
#include "littlefs_private.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Wear-leveling interval in erase cycles, littlefs's recommended range; it
 * only matters for devices that wear out, such as flash. */
#define LITTLEFS_BLOCK_CYCLES 500
/* Lookahead bitmap size in bytes; 16 bytes track 128 blocks per scan. */
#define LITTLEFS_LOOKAHEAD_SIZE 16U

/* A mounted volume; config.context points at its block device. */
typedef struct {
    lfs_t lfs;
    struct lfs_config config;
    struct littlefs_file *files; /* Open files, for the unlink check. */
} littlefs_volume_t;

/* An open file with its volume-relative path. */
typedef struct littlefs_file {
    lfs_file_t file;
    struct littlefs_file *next;
    char path[];
} littlefs_file_t;

int littlefs_errno(int result) {
    switch (result) {
    case LFS_ERR_NOENT:
        return ENOENT;
    case LFS_ERR_EXIST:
        return EEXIST;
    case LFS_ERR_NOTDIR:
        return ENOTDIR;
    case LFS_ERR_ISDIR:
        return EISDIR;
    case LFS_ERR_NOTEMPTY:
        return ENOTEMPTY;
    case LFS_ERR_BADF:
        return EBADF;
    case LFS_ERR_FBIG:
        return EFBIG;
    case LFS_ERR_INVAL:
        return EINVAL;
    case LFS_ERR_NOSPC:
        return ENOSPC;
    case LFS_ERR_NOMEM:
        return ENOMEM;
    case LFS_ERR_NAMETOOLONG:
        return ENAMETOOLONG;
    case LFS_ERR_CORRUPT:
    case LFS_ERR_IO:
    default:
        return EIO;
    }
}

static int fail(int result) {
    errno = littlefs_errno(result);
    return -1;
}

/* Block device callbacks. littlefs blocks are whole sectors, so offsets are 0
 * and sizes are one sector; the checks keep that assumption explicit. */
static int block_read(const struct lfs_config *config,
                      lfs_block_t block,
                      lfs_off_t offset,
                      void *buffer,
                      lfs_size_t size) {
    const block_device_t *device = config->context;
    if (offset != 0 || size != BLOCK_SECTOR_SIZE) {
        return LFS_ERR_INVAL;
    }
    return device->ops->read(device->device, block, buffer, 1) < 0 ? LFS_ERR_IO : 0;
}

static int block_prog(const struct lfs_config *config,
                      lfs_block_t block,
                      lfs_off_t offset,
                      const void *buffer,
                      lfs_size_t size) {
    const block_device_t *device = config->context;
    if (offset != 0 || size != BLOCK_SECTOR_SIZE) {
        return LFS_ERR_INVAL;
    }
    return device->ops->write(device->device, block, buffer, 1) < 0 ? LFS_ERR_IO : 0;
}

/* Sector devices overwrite in place, so a block needs no erase before a
 * program. A flash driver would erase here. */
static int block_erase(const struct lfs_config *config, lfs_block_t block) {
    (void)config;
    (void)block;
    return 0;
}

static int block_sync(const struct lfs_config *config) {
    const block_device_t *device = config->context;
    return device->ops->sync(device->device) < 0 ? LFS_ERR_IO : 0;
}

void littlefs_config_init(struct lfs_config *config, const block_device_t *block) {
    *config = (struct lfs_config){
        /* littlefs's context is non-const; the callbacks only read through it,
         * converting back to const block_device_t *. */
        .context = (void *)block,
        .read = block_read,
        .prog = block_prog,
        .erase = block_erase,
        .sync = block_sync,
        .read_size = BLOCK_SECTOR_SIZE,
        .prog_size = BLOCK_SECTOR_SIZE,
        .block_size = BLOCK_SECTOR_SIZE,
        .block_count = block->ops->sector_count(block->device),
        .block_cycles = LITTLEFS_BLOCK_CYCLES,
        .cache_size = BLOCK_SECTOR_SIZE,
        .lookahead_size = LITTLEFS_LOOKAHEAD_SIZE,
    };
}

static int littlefs_stat(void *fs, const char *path, vfs_stat_t *stat) {
    littlefs_volume_t *volume = fs;
    struct lfs_info info;
    int result = lfs_stat(&volume->lfs, path, &info);
    if (result < 0) {
        return fail(result);
    }
    bool directory = info.type == LFS_TYPE_DIR;
    *stat = (vfs_stat_t){
        .is_directory = directory,
        .is_regular = !directory,
        .size = directory ? 0U : (unsigned)info.size,
    };
    return 0;
}

static int open_flags(int flags) {
    int access = flags & O_ACCMODE;
    int result = access == O_RDONLY ? LFS_O_RDONLY : access == O_WRONLY ? LFS_O_WRONLY : LFS_O_RDWR;
    if (flags & O_CREAT) {
        result |= LFS_O_CREAT;
    }
    if (flags & O_EXCL) {
        result |= LFS_O_EXCL;
    }
    if (flags & O_TRUNC) {
        result |= LFS_O_TRUNC;
    }
    if (flags & O_APPEND) {
        result |= LFS_O_APPEND;
    }
    return result;
}

static int littlefs_open(void *fs, const char *path, int flags, void **file) {
    littlefs_volume_t *volume = fs;
    size_t size = strlen(path) + 1;
    littlefs_file_t *handle = calloc(1, sizeof(*handle) + size);
    if (!handle) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(handle->path, path, size);
    int result = lfs_file_open(&volume->lfs, &handle->file, path, open_flags(flags));
    if (result < 0) {
        free(handle);
        return fail(result);
    }
    handle->next = volume->files;
    volume->files = handle;
    *file = handle;
    return 0;
}

static int littlefs_close(void *fs, void *file) {
    littlefs_volume_t *volume = fs;
    littlefs_file_t *handle = file;
    for (littlefs_file_t **link = &volume->files; *link; link = &(*link)->next) {
        if (*link == handle) {
            *link = handle->next;
            break;
        }
    }
    /* Closing writes pending data; the handle is released even on failure. */
    int result = lfs_file_close(&volume->lfs, &handle->file);
    free(handle);
    return result < 0 ? fail(result) : 0;
}

static int littlefs_read(void *fs, void *file, void *buf, unsigned len) {
    littlefs_volume_t *volume = fs;
    if (len > INT_MAX) {
        len = INT_MAX;
    }
    lfs_ssize_t result = lfs_file_read(&volume->lfs, &((littlefs_file_t *)file)->file, buf, len);
    return result < 0 ? fail((int)result) : (int)result;
}

static int littlefs_write(void *fs, void *file, const void *buf, unsigned len) {
    littlefs_volume_t *volume = fs;
    lfs_ssize_t result = lfs_file_write(&volume->lfs, &((littlefs_file_t *)file)->file, buf, len);
    return result < 0 ? fail((int)result) : (int)result;
}

static int littlefs_lseek(void *fs, void *file, int offset, int whence) {
    littlefs_volume_t *volume = fs;
    int mode;
    if (whence == SEEK_SET) {
        mode = LFS_SEEK_SET;
    } else if (whence == SEEK_CUR) {
        mode = LFS_SEEK_CUR;
    } else if (whence == SEEK_END) {
        mode = LFS_SEEK_END;
    } else {
        errno = EINVAL;
        return -1;
    }
    lfs_soff_t result = lfs_file_seek(&volume->lfs, &((littlefs_file_t *)file)->file, offset, mode);
    if (result < 0) {
        return fail((int)result);
    }
    if (result > INT_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    return (int)result;
}

static int littlefs_fstat(void *fs, void *file, vfs_stat_t *stat) {
    littlefs_volume_t *volume = fs;
    lfs_soff_t size = lfs_file_size(&volume->lfs, &((littlefs_file_t *)file)->file);
    if (size < 0) {
        return fail((int)size);
    }
    *stat = (vfs_stat_t){.is_regular = true, .size = (unsigned)size};
    return 0;
}

static int littlefs_mkdir(void *fs, const char *path) {
    int result = lfs_mkdir(&((littlefs_volume_t *)fs)->lfs, path);
    return result < 0 ? fail(result) : 0;
}

/* Remove a file or directory after checking its type; littlefs removes both
 * and would also remove an open file. */
static int remove_path(littlefs_volume_t *volume, const char *path, bool directory) {
    vfs_stat_t stat;
    if (littlefs_stat(volume, path, &stat) < 0) {
        return -1;
    }
    if (directory != stat.is_directory) {
        errno = directory ? ENOTDIR : EISDIR;
        return -1;
    }
    for (const littlefs_file_t *file = volume->files; file; file = file->next) {
        if (strcmp(file->path, path) == 0) {
            errno = EBUSY;
            return -1;
        }
    }
    int result = lfs_remove(&volume->lfs, path);
    return result < 0 ? fail(result) : 0;
}

static int littlefs_rmdir(void *fs, const char *path) {
    return remove_path(fs, path, true);
}

static int littlefs_unlink(void *fs, const char *path) {
    return remove_path(fs, path, false);
}

static int
littlefs_list(void *fs, const char *path, vfs_directory_visitor_t visitor, void *context) {
    littlefs_volume_t *volume = fs;
    lfs_dir_t directory;
    int result = lfs_dir_open(&volume->lfs, &directory, path);
    if (result < 0) {
        return fail(result);
    }
    int status = 0;
    for (;;) {
        struct lfs_info info;
        result = lfs_dir_read(&volume->lfs, &directory, &info);
        if (result < 0) {
            status = fail(result);
            break;
        }
        if (result == 0) {
            break;
        }
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0) {
            continue;
        }
        status = visitor(info.name, info.type == LFS_TYPE_DIR, context);
        if (status != 0) {
            break;
        }
    }
    (void)lfs_dir_close(&volume->lfs, &directory);
    return status;
}

static const vfs_fs_ops_t littlefs_ops = {
    .open = littlefs_open,
    .close = littlefs_close,
    .read = littlefs_read,
    .write = littlefs_write,
    .lseek = littlefs_lseek,
    .fstat = littlefs_fstat,
    .stat = littlefs_stat,
    .mkdir = littlefs_mkdir,
    .rmdir = littlefs_rmdir,
    .unlink = littlefs_unlink,
    .list = littlefs_list,
};

int fs_littlefs_mount(const block_device_t *block, const char *path, bool format) {
    if (!block || !path) {
        errno = EINVAL;
        return -1;
    }
    if (block->ops->sector_count(block->device) < FS_LITTLEFS_MIN_SECTORS) {
        errno = EINVAL;
        return -1;
    }
    littlefs_volume_t *volume = calloc(1, sizeof(*volume));
    if (!volume) {
        errno = ENOMEM;
        return -1;
    }
    littlefs_config_init(&volume->config, block);
    int result = lfs_mount(&volume->lfs, &volume->config);
    if (result == LFS_ERR_CORRUPT && format) {
        result = lfs_format(&volume->lfs, &volume->config);
        if (result == 0) {
            result = lfs_mount(&volume->lfs, &volume->config);
        }
    }
    if (result < 0) {
        /* A device without a valid volume reports corruption. */
        errno = result == LFS_ERR_CORRUPT ? ENODEV : littlefs_errno(result);
        free(volume);
        return -1;
    }
    if (vfs_mount(path, &littlefs_ops, volume) < 0) {
        int error = errno;
        (void)lfs_unmount(&volume->lfs);
        free(volume);
        errno = error;
        return -1;
    }
    return 0;
}
