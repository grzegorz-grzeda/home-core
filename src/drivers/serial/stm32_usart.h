/* SPDX-License-Identifier: MIT */
/* STM32 USART with interrupt-driven receive and polled transmit, shared by the
 * STM32F1 and STM32F4 families. */
#ifndef HOMECORE_DRIVERS_STM32_USART_H
#define HOMECORE_DRIVERS_STM32_USART_H

#include "homecore/drivers/console.h"
#include "homecore/vfs/vfs.h"
#include "rx_ring.h"
#include <stdbool.h>
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    uintptr_t base;    /* USART register block address */
    const char *path;  /* VFS node name, such as "/dev/uart0" */
    uint32_t clock_hz; /* Frequency of the bus clocking this USART */
    uint32_t baud;     /* Line rate; 8 data bits, no parity, 1 stop bit */
    uint32_t irq;      /* NVIC interrupt number */
} stm32_usart_config_t;

/* Per-instance state (RAM). The generator sets config; the driver owns the rest. */
typedef struct {
    const stm32_usart_config_t *config;
    vfs_node_t node;
    rx_ring_t rx;
    bool ready;
} stm32_usart_t;

/* Program the USART, enable its receive interrupt, and register config->path.
 * The board must already have enabled the USART clock and configured its pins. */
void stm32_usart_init(stm32_usart_t *dev);

/* Receive interrupt handler; called through dt_irq_dispatch(). */
void stm32_usart_isr(stm32_usart_t *dev);

extern const console_ops_t stm32_usart_console_ops;

#endif /* HOMECORE_DRIVERS_STM32_USART_H */
