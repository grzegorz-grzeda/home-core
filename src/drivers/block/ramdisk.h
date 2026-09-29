/* SPDX-License-Identifier: MIT */
/* Block device backed by a statically allocated RAM buffer. Its contents are
 * lost at reset. */
#ifndef HOMECORE_DRIVERS_RAMDISK_H
#define HOMECORE_DRIVERS_RAMDISK_H

#include "homecore/drivers/block.h"
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    uint8_t *data; /* Buffer of size bytes, generated in .bss */
    uint32_t size; /* Buffer size in bytes, a multiple of BLOCK_SECTOR_SIZE */
} ramdisk_config_t;

/* Per-instance state (RAM). The ramdisk keeps nothing beyond its config. */
typedef struct {
    const ramdisk_config_t *config;
} ramdisk_t;

/* Nothing to initialize: the buffer starts zeroed, which holds no filesystem. */
void ramdisk_init(ramdisk_t *dev);

extern const block_ops_t ramdisk_block_ops;

#endif /* HOMECORE_DRIVERS_RAMDISK_H */
