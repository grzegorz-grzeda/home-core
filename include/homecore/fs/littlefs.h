/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief littlefs volumes mounted into the VFS.
 */
#ifndef HOMECORE_FS_LITTLEFS_H
#define HOMECORE_FS_LITTLEFS_H

#include "homecore/drivers/block.h"
#include <stdbool.h>

/**
 * @defgroup littlefs littlefs filesystem
 * @ingroup filesystems
 * @brief Power-loss resilient volumes on block devices, through littlefs v2.11.
 *
 * littlefs keeps metadata in pairs of blocks updated copy-on-write, so a reset
 * at any point leaves either the previous or the new state of a file, never a
 * mix, and the volume always mounts. It also levels wear over the device's
 * blocks. It is the filesystem for storage HomeCore owns; its on-disk format
 * cannot be read by PCs, so removable media use @ref fat instead.
 *
 * @section littlefs_mounting Mounting
 *
 * A board's `mounts` section generates the fs_littlefs_mount() call in
 * `dt_mount_all()`, and firmware compiles littlefs only when some board mount
 * names `fs: littlefs`:
 *
 * @code{.yaml}
 * devices:
 *   ram0: {compatible: "homecore,ramdisk", size: 32K}
 * mounts:
 *   /ram: {device: ram0, fs: littlefs, format: true}
 * @endcode
 *
 * The volume needs at least #FS_LITTLEFS_MIN_SECTORS sectors, so a ramdisk of
 * a few kilobytes works where FAT needs 64 KB. With `format: true`, a device
 * without a valid volume is formatted; an existing volume is kept.
 *
 * @section littlefs_geometry Geometry
 *
 * | Setting | Value | Reason |
 * | --- | --- | --- |
 * | Block size | 512 bytes | One block device sector |
 * | Read and program size | 512 bytes | Block devices transfer whole sectors |
 * | Cache size | 512 bytes | One sector |
 * | Lookahead | 16 bytes | Tracks 128 blocks per allocation scan |
 * | Block cycles | 500 | Wear-leveling interval, for devices that wear out |
 * | Name length | 127 bytes | `LFS_NAME_MAX`, the VFS path limit |
 *
 * Erasing is a no-op: sector devices such as ramdisks and SD cards overwrite
 * in place. A future flash block device would erase in its driver.
 *
 * @section littlefs_behavior File behavior
 *
 * - Names are case-sensitive, like RAM files.
 * - An open file cannot be removed (`EBUSY`). littlefs itself allows it, so
 *   the glue tracks open files per volume.
 * - `O_APPEND` writes at the end of the file every time.
 * - Seeking beyond the end is allowed, and a later write fills the gap with
 *   zeros, as for RAM files.
 * - Data reaches the device when a file is closed or its cache fills; a reset
 *   before vfs_close() keeps the previous contents.
 * - A full volume fails writes with `ENOSPC`; vfs_close() can also report it
 *   for data that could not be written.
 *
 * @section littlefs_memory Memory
 *
 * All littlefs state is allocated from the heap. Sizes measured on Cortex-M:
 *
 * | Object | Bytes | Lifetime |
 * | --- | --- | --- |
 * | Volume: `lfs_t`, configuration, two caches, lookahead | about 1260 | Until reset |
 * | Open file: `lfs_file_t`, path, 512-byte cache | about 600 + path | Until vfs_close() |
 *
 * Volumes live until reset because there is no unmount.
 *
 * The largest stack objects are `struct lfs_info` (136 bytes) and `lfs_dir_t`
 * (52 bytes). Upstream littlefs avoids recursion, so its stack use is bounded.
 *
 * @section littlefs_errors Errors
 *
 * | littlefs result | `errno` |
 * | --- | --- |
 * | `LFS_ERR_NOENT` | `ENOENT` |
 * | `LFS_ERR_EXIST` | `EEXIST` |
 * | `LFS_ERR_NOTDIR` | `ENOTDIR` |
 * | `LFS_ERR_ISDIR` | `EISDIR` |
 * | `LFS_ERR_NOTEMPTY` | `ENOTEMPTY` |
 * | `LFS_ERR_NOSPC` | `ENOSPC` |
 * | `LFS_ERR_NOMEM` | `ENOMEM` |
 * | `LFS_ERR_FBIG` | `EFBIG` |
 * | `LFS_ERR_INVAL` | `EINVAL` |
 * | `LFS_ERR_NAMETOOLONG` | `ENAMETOOLONG` |
 * | `LFS_ERR_BADF` | `EBADF` |
 * | `LFS_ERR_CORRUPT` | `ENODEV` when mounting, otherwise `EIO` |
 * | `LFS_ERR_IO`, others | `EIO` |
 *
 * The values are mapped explicitly because littlefs uses Linux numbers, which
 * differ from newlib's for several codes.
 *
 * @section littlefs_source Source
 *
 * littlefs is the `external/littlefs` submodule, pinned to v2.11.3. It is
 * built with `LFS_NAME_MAX=127` and its diagnostic printing disabled; its
 * assertions follow `NDEBUG`. The VFS glue is
 * `src/subsystems/fs/littlefs/littlefs.c`. Upstream references:
 * [README](https://github.com/littlefs-project/littlefs/blob/v2.11.3/README.md),
 * [design](https://github.com/littlefs-project/littlefs/blob/v2.11.3/DESIGN.md),
 * and [on-disk format](https://github.com/littlefs-project/littlefs/blob/v2.11.3/SPEC.md).
 * @{
 */

/**
 * @brief Smallest block device, in sectors, that fs_littlefs_mount() accepts.
 *
 * Two blocks hold the superblock pair; the rest hold directories and file data.
 */
#define FS_LITTLEFS_MIN_SECTORS 4U

/**
 * @brief Mount the littlefs volume on a block device at a new VFS mount point.
 *
 * Each 512-byte sector is one littlefs block. The volume allocates its state
 * and two 512-byte caches from the heap; every open file allocates another
 * 512-byte cache.
 *
 * @param block  Block device holding the volume. Must remain valid for the
 *               rest of the program.
 * @param path   Absolute mount-point path; see vfs_mount().
 * @param format Create a new volume when the device holds no valid littlefs
 *               volume. An existing volume is never reformatted.
 *
 * @retval 0  The volume is mounted.
 * @retval -1 `errno` is `EINVAL` (missing argument or fewer than
 *            #FS_LITTLEFS_MIN_SECTORS sectors), `ENODEV` (no volume and
 *            @p format is false), `ENOMEM`, `EIO`, or set by vfs_mount().
 */
int fs_littlefs_mount(const block_device_t *block, const char *path, bool format);

/** @} */

#endif /* HOMECORE_FS_LITTLEFS_H */
