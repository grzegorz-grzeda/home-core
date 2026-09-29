// SPDX-License-Identifier: MIT
/* FAT volumes in the VFS: translates vfs_fs_ops_t calls into FatFs calls on
 * logical drive "N:", and FatFs results into errno values. */
#include "homecore/fs/fat.h"
#include "fat_private.h"
#include "ff.h"
#include "homecore/vfs/vfs.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* "N:" drive prefix followed by a VFS-relative path. */
#define FAT_PATH_CAPACITY (VFS_PATH_CAPACITY + 2U)

/* A mounted volume; one FatFs logical drive. */
typedef struct {
    FATFS fs;
    unsigned drive;
} fat_volume_t;

/* An open file. append repeats the O_APPEND seek before every write. */
typedef struct {
    FIL fil;
    bool append;
} fat_file_t;

static fat_volume_t *volumes[FF_VOLUMES];

static int fail(FRESULT result) {
    static const int errors[] = {
        [FR_DISK_ERR] = EIO,
        [FR_INT_ERR] = EIO,
        [FR_NOT_READY] = EIO,
        [FR_NO_FILE] = ENOENT,
        [FR_NO_PATH] = ENOENT,
        [FR_INVALID_NAME] = EINVAL,
        [FR_DENIED] = EACCES,
        [FR_EXIST] = EEXIST,
        [FR_INVALID_OBJECT] = EBADF,
        [FR_WRITE_PROTECTED] = EROFS,
        [FR_INVALID_DRIVE] = ENXIO,
        [FR_NOT_ENABLED] = ENXIO,
        [FR_NO_FILESYSTEM] = ENODEV,
        [FR_MKFS_ABORTED] = EINVAL,
        [FR_TIMEOUT] = EBUSY,
        [FR_LOCKED] = EBUSY,
        [FR_NOT_ENOUGH_CORE] = ENOMEM,
        [FR_TOO_MANY_OPEN_FILES] = EMFILE,
        [FR_INVALID_PARAMETER] = EINVAL,
    };
    unsigned index = (unsigned)result;
    errno = (index < sizeof(errors) / sizeof(errors[0]) && errors[index]) ? errors[index] : EIO;
    return -1;
}

/* Prefix a VFS-relative path with the volume's drive number. */
static int fat_path(const fat_volume_t *volume, const char *path, char out[FAT_PATH_CAPACITY]) {
    int length = snprintf(out, FAT_PATH_CAPACITY, "%u:%s", volume->drive, path);
    if (length < 0 || (unsigned)length >= FAT_PATH_CAPACITY) {
        errno = ENAMETOOLONG;
        return -1;
    }
    return 0;
}

/* FatFs reports no information for the root directory; handle it here. */
static int stat_path(fat_volume_t *volume, const char *path, FILINFO *info) {
    if (strcmp(path, "/") == 0) {
        memset(info, 0, sizeof(*info));
        info->fattrib = AM_DIR;
        return 0;
    }
    char full[FAT_PATH_CAPACITY];
    if (fat_path(volume, path, full) < 0) {
        return -1;
    }
    FRESULT result = f_stat(full, info);
    return result == FR_OK ? 0 : fail(result);
}

static int fat_stat(void *fs, const char *path, vfs_stat_t *stat) {
    FILINFO info;
    if (stat_path(fs, path, &info) < 0) {
        return -1;
    }
    bool directory = (info.fattrib & AM_DIR) != 0;
    *stat = (vfs_stat_t){
        .is_directory = directory,
        .is_regular = !directory,
        .size = directory ? 0U : (unsigned)info.fsize,
    };
    return 0;
}

static BYTE open_mode(int flags) {
    BYTE mode = 0;
    int access = flags & O_ACCMODE;
    if (access == O_RDONLY || access == O_RDWR) {
        mode |= FA_READ;
    }
    if (access == O_WRONLY || access == O_RDWR) {
        mode |= FA_WRITE;
    }
    if ((flags & O_CREAT) && (flags & O_EXCL)) {
        mode |= FA_CREATE_NEW;
    } else if ((flags & O_CREAT) && (flags & O_TRUNC)) {
        mode |= FA_CREATE_ALWAYS;
    } else if (flags & O_CREAT) {
        mode |= FA_OPEN_ALWAYS;
    }
    return mode;
}

static int fat_open(void *fs, const char *path, int flags, void **file) {
    fat_volume_t *volume = fs;
    FILINFO info;
    if (stat_path(volume, path, &info) == 0 && (info.fattrib & AM_DIR)) {
        errno = EISDIR;
        return -1;
    }
    char full[FAT_PATH_CAPACITY];
    if (fat_path(volume, path, full) < 0) {
        return -1;
    }
    fat_file_t *handle = calloc(1, sizeof(*handle));
    if (!handle) {
        errno = ENOMEM;
        return -1;
    }
    FRESULT result = f_open(&handle->fil, full, open_mode(flags));
    if (result == FR_OK && (flags & O_TRUNC) && !(flags & O_CREAT)) {
        result = f_truncate(&handle->fil);
        if (result != FR_OK) {
            (void)f_close(&handle->fil);
        }
    }
    if (result != FR_OK) {
        free(handle);
        return fail(result);
    }
    handle->append = (flags & O_APPEND) != 0;
    *file = handle;
    return 0;
}

static int fat_close(void *fs, void *file) {
    (void)fs;
    FRESULT result = f_close(&((fat_file_t *)file)->fil);
    free(file);
    return result == FR_OK ? 0 : fail(result);
}

static int fat_read(void *fs, void *file, void *buf, unsigned len) {
    (void)fs;
    if (len > INT_MAX) {
        len = INT_MAX;
    }
    UINT count;
    FRESULT result = f_read(&((fat_file_t *)file)->fil, buf, len, &count);
    return result == FR_OK ? (int)count : fail(result);
}

static int fat_write(void *fs, void *file, const void *buf, unsigned len) {
    (void)fs;
    fat_file_t *handle = file;
    if (handle->append) {
        FRESULT result = f_lseek(&handle->fil, f_size(&handle->fil));
        if (result != FR_OK) {
            return fail(result);
        }
    }
    UINT count;
    FRESULT result = f_write(&handle->fil, buf, len, &count);
    if (result != FR_OK) {
        return fail(result);
    }
    if (count == 0) {
        errno = ENOSPC;
        return -1;
    }
    return (int)count;
}

static int fat_lseek(void *fs, void *file, int offset, int whence) {
    (void)fs;
    FIL *fil = &((fat_file_t *)file)->fil;
    int64_t position = offset;
    if (whence == SEEK_CUR) {
        position += f_tell(fil);
    } else if (whence == SEEK_END) {
        position += f_size(fil);
    } else if (whence != SEEK_SET) {
        position = -1;
    }
    /* FatFs would extend a writable file with uninitialized clusters. */
    if (position < 0 || position > (int64_t)f_size(fil)) {
        errno = EINVAL;
        return -1;
    }
    if (position > INT_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    FRESULT result = f_lseek(fil, (FSIZE_t)position);
    return result == FR_OK ? (int)position : fail(result);
}

static int fat_fstat(void *fs, void *file, vfs_stat_t *stat) {
    (void)fs;
    *stat = (vfs_stat_t){.is_regular = true, .size = (unsigned)f_size(&((fat_file_t *)file)->fil)};
    return 0;
}

static int fat_mkdir(void *fs, const char *path) {
    char full[FAT_PATH_CAPACITY];
    if (fat_path(fs, path, full) < 0) {
        return -1;
    }
    FRESULT result = f_mkdir(full);
    return result == FR_OK ? 0 : fail(result);
}

/* Remove a file or directory after checking its type; FatFs removes both. */
static int remove_path(fat_volume_t *volume, const char *path, bool directory) {
    FILINFO info;
    if (stat_path(volume, path, &info) < 0) {
        return -1;
    }
    if (directory != ((info.fattrib & AM_DIR) != 0)) {
        errno = directory ? ENOTDIR : EISDIR;
        return -1;
    }
    char full[FAT_PATH_CAPACITY];
    if (fat_path(volume, path, full) < 0) {
        return -1;
    }
    FRESULT result = f_unlink(full);
    if (result == FR_DENIED && directory) {
        errno = ENOTEMPTY;
        return -1;
    }
    return result == FR_OK ? 0 : fail(result);
}

static int fat_rmdir(void *fs, const char *path) {
    return remove_path(fs, path, true);
}

static int fat_unlink(void *fs, const char *path) {
    return remove_path(fs, path, false);
}

static int fat_list(void *fs, const char *path, vfs_directory_visitor_t visitor, void *context) {
    char full[FAT_PATH_CAPACITY];
    if (fat_path(fs, path, full) < 0) {
        return -1;
    }
    DIR directory;
    FRESULT result = f_opendir(&directory, full);
    if (result != FR_OK) {
        FILINFO info;
        if (result == FR_NO_PATH && stat_path(fs, path, &info) == 0) {
            errno = ENOTDIR; /* FatFs reports a file as a missing directory. */
            return -1;
        }
        return fail(result);
    }
    int status = 0;
    for (;;) {
        FILINFO info;
        result = f_readdir(&directory, &info);
        if (result != FR_OK) {
            status = fail(result);
            break;
        }
        if (info.fname[0] == '\0') {
            break;
        }
        status = visitor(info.fname, (info.fattrib & AM_DIR) != 0, context);
        if (status != 0) {
            break;
        }
    }
    (void)f_closedir(&directory);
    return status;
}

static const vfs_fs_ops_t fat_ops = {
    .open = fat_open,
    .close = fat_close,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .fstat = fat_fstat,
    .stat = fat_stat,
    .mkdir = fat_mkdir,
    .rmdir = fat_rmdir,
    .unlink = fat_unlink,
    .list = fat_list,
};

/* Create a FAT volume filling the device. FatFs picks FAT12 or FAT16 by size. */
static FRESULT format(const char *drive) {
    void *work = malloc(FF_MAX_SS);
    if (!work) {
        return FR_NOT_ENOUGH_CORE;
    }
    /* One FAT and 128 root entries (8 sectors) keep small volumes usable. */
    static const MKFS_PARM options = {.fmt = FM_ANY | FM_SFD, .n_fat = 1, .n_root = 128};
    FRESULT result = f_mkfs(drive, &options, work, FF_MAX_SS);
    free(work);
    return result;
}

int fs_fat_mount(const block_device_t *block, const char *path, bool format_empty) {
    if (!block || !path) {
        errno = EINVAL;
        return -1;
    }
    unsigned drive = 0;
    while (drive < FF_VOLUMES && volumes[drive]) {
        drive++;
    }
    if (drive == FF_VOLUMES) {
        errno = ENOSPC;
        return -1;
    }
    fat_volume_t *volume = calloc(1, sizeof(*volume));
    if (!volume) {
        errno = ENOMEM;
        return -1;
    }
    volume->drive = drive;
    char name[] = {(char)('0' + drive), ':', '\0'};
    fat_disk_attach(drive, block);
    FRESULT result = f_mount(&volume->fs, name, 1);
    if (result == FR_NO_FILESYSTEM && format_empty) {
        result = format(name);
        if (result == FR_OK) {
            result = f_mount(&volume->fs, name, 1);
        }
    }
    if (result == FR_OK && vfs_mount(path, &fat_ops, volume) == 0) {
        volumes[drive] = volume;
        return 0;
    }
    int error = errno;
    if (result != FR_OK) {
        (void)fail(result);
        error = errno;
    }
    (void)f_mount(NULL, name, 0);
    fat_disk_attach(drive, NULL);
    free(volume);
    errno = error;
    return -1;
}
