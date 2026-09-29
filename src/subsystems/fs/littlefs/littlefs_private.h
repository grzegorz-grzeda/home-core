/* SPDX-License-Identifier: MIT */
/* littlefs configuration shared by the VFS glue and its host tests. */
#ifndef HOMECORE_FS_LITTLEFS_PRIVATE_H
#define HOMECORE_FS_LITTLEFS_PRIVATE_H

#include "homecore/drivers/block.h"
#include "lfs.h"

/* Fill a littlefs configuration for a block device: one block per sector,
 * whole-sector reads and programs, and heap-allocated caches. The block device
 * must outlive every use of the configuration. */
void littlefs_config_init(struct lfs_config *config, const block_device_t *block);

/* Convert a negative littlefs result to an errno value. */
int littlefs_errno(int result);

#endif /* HOMECORE_FS_LITTLEFS_PRIVATE_H */
