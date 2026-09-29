/* SPDX-License-Identifier: MIT */
/* Stellaris (LM3S) UART in polling mode, using the reset line configuration. */
#ifndef HOMECORE_DRIVERS_STELLARIS_UART_H
#define HOMECORE_DRIVERS_STELLARIS_UART_H

#include "homecore/drivers/console.h"
#include "homecore/vfs/vfs.h"
#include <stdbool.h>
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    uintptr_t base;   /* UART register block address */
    const char *path; /* VFS node name, such as "/dev/uart0" */
} stellaris_uart_config_t;

/* Per-instance state (RAM). The generator sets config; the driver owns the rest. */
typedef struct {
    const stellaris_uart_config_t *config;
    vfs_node_t node;
    bool ready;
} stellaris_uart_t;

/* Register config->path. The UART keeps its reset configuration, which QEMU
 * provides ready to use; physical Stellaris boards are not supported yet. */
void stellaris_uart_init(stellaris_uart_t *dev);

extern const console_ops_t stellaris_uart_console_ops;

#endif /* HOMECORE_DRIVERS_STELLARIS_UART_H */
