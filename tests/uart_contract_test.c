/* SPDX-License-Identifier: MIT */
#include "homecore/vfs/vfs.h"
#include "homecore/board/board.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

static vfs_node_t *registered_console;
void vfs_register_node(vfs_node_t *node) {
    if (!registered_console) {
        registered_console = node;
    }
}

#ifdef TEST_STM32
static unsigned reads;
static unsigned writes;
static unsigned char last_byte;
int board_uart_getc(void) {
    ++reads;
    return 0xff;
}
void board_uart_putc(char c) {
    ++writes;
    last_byte = (unsigned char)c;
}
#include "../src/soc/st/stm32f407/soc.c"
#else
/* Replace only the MMIO device header; exercise the real LM3S callbacks. */
#define HOME_CORE_SOC_CMSIS_H
typedef struct {
    uint32_t FR;
    uint32_t DR;
} UART0_Type;
static UART0_Type fake_uart;
#define UART0 (&fake_uart)
#define UART1 (&fake_uart)
#define UART2 (&fake_uart)
#include "../src/soc/ti/lm3s6965/soc.c"
#endif

int main(void) {
    soc_init();
    assert(registered_console);
    char byte = 'x';
#ifndef TEST_STM32
    /* Empty RX/full TX ensure invalid requests cannot reach polling loops. */
    fake_uart.FR = UART_FR_RXFE | UART_FR_TXFF;
    fake_uart.DR = 0x55;
#endif
    int result = registered_console->ops.read(registered_console, &byte, 0);
    assert(result == 0 && byte == 'x');
    result = registered_console->ops.read(registered_console, NULL, 0);
    assert(result == 0);
    result = registered_console->ops.write(registered_console, NULL, 0);
    assert(result == 0);
    errno = 0;
    result = registered_console->ops.read(registered_console, NULL, 1);
    assert(result == -1 && errno == EFAULT);
    errno = 0;
    result = registered_console->ops.write(registered_console, NULL, 1);
    assert(result == -1 && errno == EFAULT);
    errno = 0;
    result = registered_console->ops.write(registered_console, &byte, (unsigned)INT_MAX + 1U);
    assert(result == -1 && errno == EOVERFLOW);
#ifdef TEST_STM32
    assert(reads == 0 && writes == 0);
#else
    assert(fake_uart.DR == 0x55);
    fake_uart.FR = 0;
    fake_uart.DR = 0xff;
#endif
    result = registered_console->ops.read(registered_console, &byte, 1);
    assert(result == 1 && (unsigned char)byte == 0xff);
    result = registered_console->ops.write(registered_console, &byte, 1);
    assert(result == 1);
#ifdef TEST_STM32
    assert(reads == 1 && writes == 1 && last_byte == 0xff);
#else
    assert(fake_uart.DR == 0xff);
#endif
    puts("PASS: UART zero-length, invalid buffers, oversized writes, byte transfers");
    return 0;
}
