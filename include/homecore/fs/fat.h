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
 * @ingroup filesystems
 * @brief FAT12/16/32 volumes on block devices, through ChaN's FatFs R0.16.
 *
 * FAT is the format PCs, cameras, and SD cards use, so it suits removable
 * media that must be read elsewhere. It is not safe against power loss: a
 * reset during a write can leave the volume inconsistent. For data that must
 * survive resets on a device HomeCore owns, use @ref littlefs.
 *
 * @section fat_mounting Mounting
 *
 * A board's `mounts` section generates the fs_fat_mount() call in
 * `dt_mount_all()`, and firmware compiles FatFs only when some board mount
 * names `fs: fat`:
 *
 * @code{.yaml}
 * devices:
 *   ram0: {compatible: "homecore,ramdisk", size: 64K}
 * mounts:
 *   /ram: {device: ram0, fs: fat, format: true}
 * @endcode
 *
 * Each mount uses one FatFs logical drive (`0:`, `1:`, ...), up to
 * `CONFIG_HOMECORE_FS_FAT_VOLUMES`. With `format: true`, a device without a
 * FAT volume is formatted with `f_mkfs()`: no partition table, one FAT, 128
 * root-directory entries, and FAT12 or FAT16 chosen by size. Volumes need at
 * least 128 sectors (64 KB), the smallest size FatFs formats or mounts.
 *
 * @section fat_behavior File behavior
 *
 * - Names are case-insensitive and stored in code page 437. Names longer than
 *   8.3 need `CONFIG_HOMECORE_FS_FAT_LFN` (default on); names are limited to
 *   127 bytes, the VFS path limit.
 * - Files have no timestamps (`FF_FS_NORTC`); FatFs records 2026-01-01.
 * - An open file cannot be removed (`EBUSY`), through FatFs's lock table
 *   (`FF_FS_LOCK`), which is sized for every VFS descriptor plus one listing.
 * - `O_APPEND` moves to the end of the file before every write.
 * - Seeking beyond the end of a file fails with `EINVAL`: FatFs would extend
 *   the file with uninitialized clusters instead of zeros.
 * - A write that fills the volume returns a short count, and the next write
 *   fails with `ENOSPC`.
 *
 * @section fat_memory Memory
 *
 * All FatFs state is allocated from the heap. Sizes measured on Cortex-M:
 *
 * | Object | Bytes | Lifetime |
 * | --- | --- | --- |
 * | Volume (`FATFS` with a 512-byte sector window) | about 570 | Until reset; there is no unmount |
 * | Open file (`FIL` with a 512-byte sector buffer) | about 560 | Until vfs_close() |
 * | Long-name working buffer | 512 | During each path operation, with LFN |
 * | Format work buffer | 512 | During formatting |
 *
 * `CONFIG_HOMECORE_FS_FAT_TINY` removes the per-file sector buffer, making
 * files share the volume's window, at some speed cost. The largest stack
 * object is `FILINFO` (152 bytes), kept small by limiting long names to the
 * 127-byte VFS path length.
 *
 * @section fat_errors Errors
 *
 * | FatFs result | `errno` |
 * | --- | --- |
 * | `FR_NO_FILE`, `FR_NO_PATH` | `ENOENT` (`ENOTDIR` when listing a file) |
 * | `FR_EXIST` | `EEXIST` |
 * | `FR_DENIED` | `EACCES`; `ENOTEMPTY` when removing a directory |
 * | `FR_LOCKED`, `FR_TIMEOUT` | `EBUSY` |
 * | `FR_NO_FILESYSTEM` | `ENODEV` |
 * | `FR_INVALID_NAME`, `FR_INVALID_PARAMETER`, `FR_MKFS_ABORTED` | `EINVAL` |
 * | `FR_NOT_ENOUGH_CORE` | `ENOMEM` |
 * | `FR_TOO_MANY_OPEN_FILES` | `EMFILE` |
 * | `FR_WRITE_PROTECTED` | `EROFS` |
 * | `FR_INVALID_DRIVE`, `FR_NOT_ENABLED` | `ENXIO` |
 * | `FR_INVALID_OBJECT` | `EBADF` |
 * | `FR_DISK_ERR`, `FR_INT_ERR`, `FR_NOT_READY`, others | `EIO` |
 *
 * @section fat_source Source
 *
 * FatFs is vendored unmodified in `external/fatfs` (see its `README.md` for
 * the archive checksum). HomeCore's configuration is
 * `src/subsystems/fs/fat/ffconf.h`; the disk interface is
 * `src/subsystems/fs/fat/diskio.c`, and the VFS glue is `fat.c` next to it.
 * The upstream reference is the
 * [FatFs documentation](https://elm-chan.org/fsw/ff/).
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
