// SPDX-License-Identifier: MIT
#include "ramdisk.h"
#include <errno.h>
#include <stddef.h>
#include <string.h>

static uint32_t sector_count(void *device) {
    return ((const ramdisk_t *)device)->config->size / BLOCK_SECTOR_SIZE;
}

/* Return 0 for a valid transfer, or -1 with errno set to EIO when the sector
 * range does not fit the disk or EFAULT for a missing buffer. */
static int check_range(void *device, uint32_t sector, uint32_t count, const void *buf) {
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    uint32_t sectors = sector_count(device);
    if (sector > sectors || count > sectors - sector) {
        errno = EIO;
        return -1;
    }
    return 0;
}

static int ramdisk_read(void *device, uint32_t sector, void *buf, uint32_t count) {
    if (check_range(device, sector, count, buf) < 0) {
        return -1;
    }
    const uint8_t *data = ((const ramdisk_t *)device)->config->data;
    memcpy(buf, data + (size_t)sector * BLOCK_SECTOR_SIZE, (size_t)count * BLOCK_SECTOR_SIZE);
    return 0;
}

static int ramdisk_write(void *device, uint32_t sector, const void *buf, uint32_t count) {
    if (check_range(device, sector, count, buf) < 0) {
        return -1;
    }
    uint8_t *data = ((const ramdisk_t *)device)->config->data;
    memcpy(data + (size_t)sector * BLOCK_SECTOR_SIZE, buf, (size_t)count * BLOCK_SECTOR_SIZE);
    return 0;
}

static int ramdisk_sync(void *device) {
    (void)device;
    return 0;
}

const block_ops_t ramdisk_block_ops = {
    .read = ramdisk_read,
    .write = ramdisk_write,
    .sync = ramdisk_sync,
    .sector_count = sector_count,
};

void ramdisk_init(ramdisk_t *dev) {
    (void)dev;
}
