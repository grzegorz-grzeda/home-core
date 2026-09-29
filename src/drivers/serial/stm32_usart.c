// SPDX-License-Identifier: MIT
#include "stm32_usart.h"
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

static int poll_in(void *device) {
    USART_TypeDef *usart = regs(device);
    for (;;) {
        uint32_t status = usart->SR;
        if (status & (USART_SR_RXNE | RX_ERRORS)) {
            /* Reading SR then DR clears receive errors, including overrun. */
            uint32_t data = usart->DR;
            if ((status & USART_SR_RXNE) && !(status & (USART_SR_NE | USART_SR_FE | USART_SR_PE))) {
                return (int)(data & 0xffU);
            }
        }
    }
}

static int has_data(void *device) {
    return (regs(device)->SR & USART_SR_RXNE) != 0;
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
    usart->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    dev->node = (vfs_node_t){
        .name = config->path,
        .ops = {.read = usart_read, .write = usart_write},
        .driver_data = dev,
    };
    vfs_register_node(&dev->node);
    dev->ready = true;
}
