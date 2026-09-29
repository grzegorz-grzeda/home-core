// SPDX-License-Identifier: MIT
#include "stellaris_uart.h"
#include "homecore/arch/arch.h"
#include "soc_cmsis.h"
#include <errno.h>
#include <limits.h>

#define UART_FR_TXFF (1u << 5)
#define UART_FR_RXFE (1u << 4)
#define UART_FR_BUSY (1u << 3)
/* Receive and receive-timeout interrupts, in IM, MIS, and ICR. */
#define UART_INT_RX (1u << 4)
#define UART_INT_RT (1u << 6)
/* DR bits 8-11: overrun, break, parity, and framing errors for the byte. */
#define UART_DR_ERRORS (0xfu << 8)
/* Receive FIFO depth: bounds the work of one interrupt. */
#define UART_FIFO_DEPTH 16U

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

void stellaris_uart_isr(stellaris_uart_t *dev) {
    UART0_Type *uart = regs(dev);
    uart->ICR = UART_INT_RX | UART_INT_RT;
    for (unsigned i = 0; i < UART_FIFO_DEPTH && !(uart->FR & UART_FR_RXFE); ++i) {
        uint32_t data = uart->DR;
        if (!(data & UART_DR_ERRORS)) {
            rx_ring_push(&dev->rx, (uint8_t)(data & 0xffU));
        }
    }
}

/* Blocking read from the receive ring. Interrupts are masked while checking,
 * so a byte arriving between the check and WFI still wakes the core. */
static int poll_in(void *device) {
    stellaris_uart_t *dev = device;
    for (;;) {
        arch_irq_key_t key = arch_irq_lock();
        int byte = rx_ring_pop(&dev->rx);
        if (byte < 0) {
            arch_cpu_idle();
        }
        arch_irq_unlock(key);
        if (byte >= 0) {
            return byte;
        }
    }
}

static int has_data(void *device) {
    return !rx_ring_empty(&((stellaris_uart_t *)device)->rx);
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
    regs(dev)->IM |= UART_INT_RX | UART_INT_RT;
    arch_irq_enable((int)dev->config->irq);
    dev->node = (vfs_node_t){
        .name = dev->config->path,
        .ops = {.read = uart_read, .write = uart_write},
        .driver_data = dev,
    };
    vfs_register_node(&dev->node);
    dev->ready = true;
}
