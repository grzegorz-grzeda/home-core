// SPDX-License-Identifier: MIT
#include "stellaris_uart.h"
#include "soc_cmsis.h"
#include <errno.h>
#include <limits.h>

#define UART_FR_TXFF (1u << 5)
#define UART_FR_RXFE (1u << 4)
#define UART_FR_BUSY (1u << 3)

static UART0_Type *regs(const stellaris_uart_t *dev) {
    return (UART0_Type *)dev->config->base;
}

static void poll_out(void *device, char c) {
    UART0_Type *uart = regs(device);
    while (uart->FR & UART_FR_TXFF) {
        /* Poll until the transmit FIFO has space. */
    }
    uart->DR = (uint8_t)c;
}

static int poll_in(void *device) {
    UART0_Type *uart = regs(device);
    while (uart->FR & UART_FR_RXFE) {
        /* Blocking console read: wait for a received byte. */
    }
    return (int)(uart->DR & 0xffU);
}

static int has_data(void *device) {
    return (regs(device)->FR & UART_FR_RXFE) == 0;
}

static void flush(void *device) {
    while (regs(device)->FR & UART_FR_BUSY) {
        /* Wait until the transmit FIFO and shift register are empty. */
    }
}

static bool ready(void *device) {
    return ((const stellaris_uart_t *)device)->ready;
}

const console_ops_t stellaris_uart_console_ops = {
    .putc = poll_out,
    .getc = poll_in,
    .has_data = has_data,
    .flush = flush,
    .ready = ready,
};

static int uart_read(vfs_node_t *node, void *buf, unsigned len) {
    if (len == 0) {
        return 0;
    }
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    *(char *)buf = (char)poll_in(node->driver_data);
    return 1;
}

static int uart_write(vfs_node_t *node, const void *buf, unsigned len) {
    if (len == 0) {
        return 0;
    }
    if (len > INT_MAX) {
        errno = EOVERFLOW;
        return -1;
    }
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    const char *bytes = buf;
    for (unsigned i = 0; i < len; ++i) {
        poll_out(node->driver_data, bytes[i]);
    }
    return (int)len;
}

void stellaris_uart_init(stellaris_uart_t *dev) {
    dev->node = (vfs_node_t){
        .name = dev->config->path,
        .ops = {.read = uart_read, .write = uart_write},
        .driver_data = dev,
    };
    vfs_register_node(&dev->node);
    dev->ready = true;
}
