/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief FAT filesystem (FatFs) mounted into the VFS.
 */
#ifndef HOMECORE_FS_FAT_H
#define HOMECORE_FS_FAT_H

#include "homecore/drivers/block.h"
#include <stdbool.h>

/**
 * @defgroup fat FAT filesystem
 * @ingroup vfs
 * @brief FAT12/16/32 volumes on block devices, through ChaN's FatFs.
 *
 * The device description's `mounts` section generates the calls to
 * fs_fat_mount(); firmware compiles FatFs only when a board mounts a FAT
 * volume. Volumes need at least 128 sectors (64 KB), the smallest size FatFs
 * accepts. Files have no timestamps, and seeking beyond the end of a file
 * fails with `EINVAL`.
 * @{
 */

/**
 * @brief Mount the FAT volume on a block device at a new VFS mount point.
 *
 * @param block  Block device holding the volume. Must remain valid for the
 *               rest of the program.
 * @param path   Absolute mount-point path; see vfs_mount().
 * @param format Create a new volume when the device holds no FAT volume.
 *
 * @retval 0  The volume is mounted.
 * @retval -1 `errno` is `ENODEV` (no FAT volume and @p format is false),
 *            `EINVAL` (device smaller than 128 sectors), `ENOSPC` (all
 *            `CONFIG_HOMECORE_FS_FAT_VOLUMES` volumes in use), `ENOMEM`, `EIO`,
 *            or set by vfs_mount().
 */
int fs_fat_mount(const block_device_t *block, const char *path, bool format);

/** @} */

#endif /* HOMECORE_FS_FAT_H */
