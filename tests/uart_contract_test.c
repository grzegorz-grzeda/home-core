/* SPDX-License-Identifier: MIT */
/* Exercises the real serial drivers against register blocks in plain memory.
 * Build with -Itests/fakes/stellaris (default) or -DTEST_STM32
 * -Itests/fakes/stm32, plus -Isrc/drivers/serial. */
#include "homecore/arch/arch.h"
#include "homecore/vfs/vfs.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static vfs_node_t *registered;
void vfs_register_node(vfs_node_t *node) {
    registered = node;
}

/* Architecture stubs: record the enabled interrupt; masking is a no-op on the
 * host. arch_cpu_idle() is reached only if a read found the ring empty, which
 * these tests never do. */
static int enabled_irq = -1;
void arch_irq_enable(int irq) {
    enabled_irq = irq;
}
arch_irq_key_t arch_irq_lock(void) {
    return 0;
}
void arch_irq_unlock(arch_irq_key_t key) {
    (void)key;
}
void arch_cpu_idle(void) {
    assert(!"read waited on an empty receive ring");
}

#ifdef TEST_STM32
#include "../src/drivers/serial/stm32_usart.c"
static USART_TypeDef regs_block;
static const stm32_usart_config_t config = {
    .base = (uintptr_t)&regs_block,
    .path = "/dev/uart0",
    .clock_hz = 24000000U,
    .baud = 115200U,
    .irq = 37U,
};
static stm32_usart_t device = {.config = &config};
#define OPS        stm32_usart_console_ops
#define INIT()     stm32_usart_init(&device)
#define SET_IDLE() (regs_block.SR = USART_SR_TXE | USART_SR_TC)
#define SET_RX(byte)                                                                               \
    (regs_block.SR = USART_SR_TXE | USART_SR_TC | USART_SR_RXNE, regs_block.DR = (byte))
#define SET_NO_INPUT_NO_SPACE() (regs_block.SR = 0)
#define ISR()                   stm32_usart_isr(&device)
#define SET_RX_ERROR(byte)                                                                         \
    (regs_block.SR = USART_SR_TXE | USART_SR_RXNE | USART_SR_FE, regs_block.DR = (byte))
#else
#include "../src/drivers/serial/stellaris_uart.c"
static UART0_Type regs_block;
static const stellaris_uart_config_t config = {
    .base = (uintptr_t)&regs_block, .path = "/dev/uart0", .irq = 5U};
static stellaris_uart_t device = {.config = &config};
#define OPS                     stellaris_uart_console_ops
#define INIT()                  stellaris_uart_init(&device)
#define SET_IDLE()              (regs_block.FR = UART_FR_RXFE)
#define SET_RX(byte)            (regs_block.FR = 0, regs_block.DR = (byte))
#define SET_NO_INPUT_NO_SPACE() (regs_block.FR = UART_FR_RXFE | UART_FR_TXFF)
#define ISR()                   stellaris_uart_isr(&device)
/* Framing error flag in DR bit 8. */
#define SET_RX_ERROR(byte)      (regs_block.FR = 0, regs_block.DR = (byte) | (1U << 8))
#endif

/* The fake FIFO never empties: the Stellaris handler reads its full 16-entry
 * depth per interrupt, the STM32 handler one byte. */
static unsigned ring_length(void) {
    return (unsigned)(device.rx.head - device.rx.tail);
}

static void drain(void) {
    while (OPS.has_data(&device)) {
        (void)OPS.getc(&device);
    }
}

int main(void) {
    assert(!OPS.ready(&device));
    INIT();
    assert(OPS.ready(&device));
    assert(registered == &device.node && strcmp(registered->name, "/dev/uart0") == 0);
    assert(enabled_irq == (int)config.irq);
#ifdef TEST_STM32
    /* 24 MHz / 115200 rounded; 8N1 with transmitter, receiver, and RX interrupt. */
    assert(regs_block.BRR == 208U);
    assert(regs_block.CR1 == (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE));
    assert(regs_block.CR2 == 0 && regs_block.CR3 == 0);
#else
    assert(regs_block.IM == (UART_INT_RX | UART_INT_RT));
#endif

    /* Invalid requests are rejected before any wait can block. */
    SET_NO_INPUT_NO_SPACE();
    regs_block.DR = 0x55;
    char byte = 'x';
    assert(registered->ops.read(registered, &byte, 0) == 0 && byte == 'x');
    assert(registered->ops.read(registered, NULL, 0) == 0);
    assert(registered->ops.write(registered, NULL, 0) == 0);
    errno = 0;
    assert(registered->ops.read(registered, NULL, 1) == -1 && errno == EFAULT);
    errno = 0;
    assert(registered->ops.write(registered, NULL, 1) == -1 && errno == EFAULT);
    errno = 0;
    assert(registered->ops.write(registered, &byte, (unsigned)INT_MAX + 1U) == -1 &&
           errno == EOVERFLOW);
    assert(regs_block.DR == 0x55);

    /* Received bytes reach readers through the interrupt handler's ring. */
    assert(!OPS.has_data(&device));
    SET_RX(0xff);
    ISR();
    assert(OPS.has_data(&device));
    assert(registered->ops.read(registered, &byte, 1) == 1 && (unsigned char)byte == 0xff);
    drain();
    SET_IDLE();
    byte = 'A';
    assert(registered->ops.write(registered, &byte, 1) == 1 && regs_block.DR == 'A');

    /* Bytes flagged with a receive error are discarded. */
    SET_RX_ERROR('e');
    ISR();
    assert(!OPS.has_data(&device));

    /* A full ring keeps the oldest bytes and counts the rest as dropped. */
    SET_RX('o');
    while (device.rx.dropped == 0) {
        ISR();
    }
    assert(ring_length() == RX_RING_SIZE);
    drain();

    /* Console operations used by board_uart_* and panic paths. */
    SET_IDLE();
    OPS.putc(&device, 'Z');
    assert(regs_block.DR == 'Z');
    SET_RX('q');
    ISR();
    assert(OPS.getc(&device) == 'q');
    drain();
    SET_IDLE();
    OPS.flush(&device); /* Idle transmitter: must return. */
    puts(
        "PASS: serial driver init, invalid requests, interrupt receive, errors, overflow, console");
    return 0;
}
