// SPDX-License-Identifier: MIT
/* FatFs disk interface: physical drive numbers map to the block devices
 * registered by fs_fat_mount(). */
#include "ff.h" /* Before diskio.h, which uses its types. */
#include "diskio.h"
#include "fat_private.h"

static const block_device_t *drives[FF_VOLUMES];

void fat_disk_attach(unsigned drive, const block_device_t *block) {
    drives[drive] = block;
}

DSTATUS disk_status(BYTE pdrv) {
    return (pdrv < FF_VOLUMES && drives[pdrv]) ? 0 : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE pdrv) {
    /* The device description initialized the device in dt_init(). */
    return disk_status(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (disk_status(pdrv)) {
        return RES_NOTRDY;
    }
    const block_device_t *block = drives[pdrv];
    return block->ops->read(block->device, sector, buff, count) < 0 ? RES_ERROR : RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (disk_status(pdrv)) {
        return RES_NOTRDY;
    }
    const block_device_t *block = drives[pdrv];
    return block->ops->write(block->device, sector, buff, count) < 0 ? RES_ERROR : RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (disk_status(pdrv)) {
        return RES_NOTRDY;
    }
    const block_device_t *block = drives[pdrv];
    switch (cmd) {
    case CTRL_SYNC:
        return block->ops->sync(block->device) < 0 ? RES_ERROR : RES_OK;
    case GET_SECTOR_COUNT:
        *(LBA_t *)buff = block->ops->sector_count(block->device);
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1; /* Erase block size unknown. */
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
