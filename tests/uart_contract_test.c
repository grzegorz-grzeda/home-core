/* SPDX-License-Identifier: MIT */
/* Exercises the real serial drivers against register blocks in plain memory.
 * Build with -Itests/fakes/stellaris (default) or -DTEST_STM32
 * -Itests/fakes/stm32, plus -Isrc/drivers/serial. */
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

#ifdef TEST_STM32
#include "../src/drivers/serial/stm32_usart.c"
static USART_TypeDef regs_block;
static const stm32_usart_config_t config = {
    .base = (uintptr_t)&regs_block,
    .path = "/dev/uart0",
    .clock_hz = 24000000U,
    .baud = 115200U,
};
static stm32_usart_t device = {.config = &config};
#define OPS        stm32_usart_console_ops
#define INIT()     stm32_usart_init(&device)
#define SET_IDLE() (regs_block.SR = USART_SR_TXE | USART_SR_TC)
#define SET_RX(byte)                                                                               \
    (regs_block.SR = USART_SR_TXE | USART_SR_TC | USART_SR_RXNE, regs_block.DR = (byte))
#define SET_NO_INPUT_NO_SPACE() (regs_block.SR = 0)
#else
#include "../src/drivers/serial/stellaris_uart.c"
static UART0_Type regs_block;
static const stellaris_uart_config_t config = {.base = (uintptr_t)&regs_block,
                                               .path = "/dev/uart0"};
static stellaris_uart_t device = {.config = &config};
#define OPS                     stellaris_uart_console_ops
#define INIT()                  stellaris_uart_init(&device)
#define SET_IDLE()              (regs_block.FR = UART_FR_RXFE)
#define SET_RX(byte)            (regs_block.FR = 0, regs_block.DR = (byte))
#define SET_NO_INPUT_NO_SPACE() (regs_block.FR = UART_FR_RXFE | UART_FR_TXFF)
#endif

int main(void) {
    assert(!OPS.ready(&device));
    INIT();
    assert(OPS.ready(&device));
    assert(registered == &device.node && strcmp(registered->name, "/dev/uart0") == 0);
#ifdef TEST_STM32
    /* 24 MHz / 115200 rounded; 8N1 with transmitter and receiver enabled. */
    assert(regs_block.BRR == 208U);
    assert(regs_block.CR1 == (USART_CR1_UE | USART_CR1_TE | USART_CR1_RE));
    assert(regs_block.CR2 == 0 && regs_block.CR3 == 0);
#endif

    /* Invalid requests are rejected before any polling loop can block. */
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

    /* Byte transfers through the VFS node. */
    SET_RX(0xff);
    assert(OPS.has_data(&device) != 0);
    assert(registered->ops.read(registered, &byte, 1) == 1 && (unsigned char)byte == 0xff);
    SET_IDLE();
    assert(OPS.has_data(&device) == 0);
    byte = 'A';
    assert(registered->ops.write(registered, &byte, 1) == 1 && regs_block.DR == 'A');

    /* Console operations used by board_uart_* and panic paths. */
    OPS.putc(&device, 'Z');
    assert(regs_block.DR == 'Z');
    SET_RX('q');
    assert(OPS.getc(&device) == 'q');
    SET_IDLE();
    OPS.flush(&device); /* Idle transmitter: must return. */
    puts("PASS: serial driver init, invalid requests, byte transfers, console operations");
    return 0;
}
