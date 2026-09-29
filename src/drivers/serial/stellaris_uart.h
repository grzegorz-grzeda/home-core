/* SPDX-License-Identifier: MIT */
/* Stellaris (LM3S) UART with interrupt-driven receive and polled transmit,
 * using the reset line configuration. */
#ifndef HOMECORE_DRIVERS_STELLARIS_UART_H
#define HOMECORE_DRIVERS_STELLARIS_UART_H

#include "homecore/drivers/console.h"
#include "homecore/vfs/vfs.h"
#include "rx_ring.h"
#include <stdbool.h>
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    uintptr_t base;   /* UART register block address */
    const char *path; /* VFS node name, such as "/dev/uart0" */
    uint32_t irq;     /* NVIC interrupt number */
} stellaris_uart_config_t;

/* Per-instance state (RAM). The generator sets config; the driver owns the rest. */
typedef struct {
    const stellaris_uart_config_t *config;
    vfs_node_t node;
    rx_ring_t rx;
    bool ready;
} stellaris_uart_t;

/* Enable the receive interrupts and register config->path. The UART keeps its
 * reset line configuration, which QEMU provides ready to use; physical
 * Stellaris boards are not supported yet. */
void stellaris_uart_init(stellaris_uart_t *dev);

/* Receive interrupt handler; called through dt_irq_dispatch(). */
void stellaris_uart_isr(stellaris_uart_t *dev);

extern const console_ops_t stellaris_uart_console_ops;

#endif /* HOMECORE_DRIVERS_STELLARIS_UART_H */
