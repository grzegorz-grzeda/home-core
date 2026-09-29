/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief Sector-addressed block devices used by filesystems.
 */
#ifndef HOMECORE_DRIVERS_BLOCK_H
#define HOMECORE_DRIVERS_BLOCK_H

#include <stdint.h>

/**
 * @defgroup block Block devices
 * @ingroup drivers
 * @brief Storage read and written in fixed-size sectors, such as a ramdisk.
 *
 * Block devices have no VFS node. The device description's `mounts` section
 * attaches a filesystem to one; the generated code builds the ::block_device_t
 * from the driver's `<driver>_block_ops` and device instance. Operations
 * return 0 on success and -1 with `errno` set on failure, and are called only
 * from thread context, never from interrupt handlers.
 *
 * A block driver sets `block: true` in its binding. Transfers are whole
 * #BLOCK_SECTOR_SIZE sectors; a sector range outside the device fails with
 * `EIO`. There is no erase operation: current devices overwrite in place.
 *
 * | Driver | Compatible | Backing store |
 * | --- | --- | --- |
 * | Ramdisk | `homecore,ramdisk` | A `.bss` buffer sized by the `size` property; lost on reset |
 *
 * @see @ref storage "Files and storage" for how filesystems use block devices.
 * @see @ref driver_block for the drivers, their configuration, and tests.
 * @{
 */

/** @brief Sector size of every block device, in bytes. */
#define BLOCK_SECTOR_SIZE 512U

/** @brief Operations a block driver provides; every one is required. */
typedef struct {
    /** Read @p count sectors starting at @p sector into @p buf. */
    int (*read)(void *device, uint32_t sector, void *buf, uint32_t count);
    /** Write @p count sectors starting at @p sector from @p buf. */
    int (*write)(void *device, uint32_t sector, const void *buf, uint32_t count);
    /** Finish every pending write. */
    int (*sync)(void *device);
    /** Return the device size in sectors. */
    uint32_t (*sector_count)(void *device);
} block_ops_t;

/** @brief A block device: driver operations bound to one device instance. */
typedef struct {
    /** Operations of the device's driver. */
    const block_ops_t *ops;
    /** Driver instance passed to every operation. */
    void *device;
} block_device_t;

/** @} */

#endif /* HOMECORE_DRIVERS_BLOCK_H */
