// SPDX-License-Identifier: MIT
#include "stm32_usart.h"
#include "homecore/arch/arch.h"
#include "soc_cmsis.h"
#include <errno.h>
#include <limits.h>

#define RX_ERRORS (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)

static USART_TypeDef *regs(const stm32_usart_t *dev) {
    return (USART_TypeDef *)dev->config->base;
}

static void poll_out(void *device, char c) {
    USART_TypeDef *usart = regs(device);
    while (!(usart->SR & USART_SR_TXE)) {
        /* Poll until the transmit register can accept a byte. */
    }
    usart->DR = (uint8_t)c;
}

void stm32_usart_isr(stm32_usart_t *dev) {
    USART_TypeDef *usart = regs(dev);
    uint32_t status = usart->SR;
    if (status & (USART_SR_RXNE | RX_ERRORS)) {
        /* Reading SR then DR clears the request and receive errors, including
         * overrun. Bytes with noise, framing, or parity errors are discarded. */
        uint32_t data = usart->DR;
        if ((status & USART_SR_RXNE) && !(status & (USART_SR_NE | USART_SR_FE | USART_SR_PE))) {
            rx_ring_push(&dev->rx, (uint8_t)(data & 0xffU));
        }
    }
}

/* Blocking read from the receive ring. Interrupts are masked while checking,
 * so a byte arriving between the check and WFI still wakes the core. */
static int poll_in(void *device) {
    stm32_usart_t *dev = device;
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
    return !rx_ring_empty(&((stm32_usart_t *)device)->rx);
}

static void flush(void *device) {
    while (!(regs(device)->SR & USART_SR_TC)) {
        /* Wait until the last byte has left the shift register. */
    }
}

static bool ready(void *device) {
    return ((const stm32_usart_t *)device)->ready;
}

const console_ops_t stm32_usart_console_ops = {
    .putc = poll_out,
    .getc = poll_in,
    .has_data = has_data,
    .flush = flush,
    .ready = ready,
};

static int usart_read(vfs_node_t *node, void *buf, unsigned len) {
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

static int usart_write(vfs_node_t *node, const void *buf, unsigned len) {
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

void stm32_usart_init(stm32_usart_t *dev) {
    const stm32_usart_config_t *config = dev->config;
    USART_TypeDef *usart = regs(dev);
    /* Oversampling by 16: BRR is the rounded bus-clock divisor. */
    usart->BRR = (config->clock_hz + config->baud / 2U) / config->baud;
    usart->CR2 = 0;
    usart->CR3 = 0;
    usart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;
    arch_irq_enable((int)config->irq);
    dev->node = (vfs_node_t){
        .name = config->path,
        .ops = {.read = usart_read, .write = usart_write},
        .driver_data = dev,
    };
    vfs_register_node(&dev->node);
    dev->ready = true;
}
