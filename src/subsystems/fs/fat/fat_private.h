/* SPDX-License-Identifier: MIT */
/* Shared between the FAT VFS glue and the FatFs disk interface. */
#ifndef HOMECORE_FS_FAT_PRIVATE_H
#define HOMECORE_FS_FAT_PRIVATE_H

#include "homecore/drivers/block.h"

/* Route FatFs physical drive `drive` to `block`; NULL detaches it. */
void fat_disk_attach(unsigned drive, const block_device_t *block);

#endif /* HOMECORE_FS_FAT_PRIVATE_H */
