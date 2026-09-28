/**
 * MIT License
 *
 * Copyright (c) 2026 Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*---------------------------------------------------------------------------*/
#include "homecore/board/board.h"
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#include "soc_cmsis.h"
/*---------------------------------------------------------------------------*/
#define UART_FR_TXFF (1u << 5)
#define UART_FR_RXFE (1u << 4)
/*---------------------------------------------------------------------------*/
/* QEMU lm3s6965evb reset clock: 200 MHz / 16. No clock changes yet. */
uint32_t board_cpu_clock_hz(void) {
    return 12500000U;
}

void board_init(void) {
}
/*---------------------------------------------------------------------------*/
void board_uart_putc(char c) {
    while (UART0->FR & UART_FR_TXFF) {
        /* Poll until the transmit FIFO has space. */
    }

    UART0->DR = (uint32_t)c;
}
/*---------------------------------------------------------------------------*/
int board_uart_has_data(void) {
    return (UART0->FR & UART_FR_RXFE) == 0;
}
/*---------------------------------------------------------------------------*/
int board_uart_getc(void) {
    while (!board_uart_has_data()) {
        /* Blocking console read: wait for a received byte. */
    }

    return (int)(UART0->DR & 0xff);
}
/*---------------------------------------------------------------------------*/
void board_panic(const char *msg) {
    while (*msg) {
        board_uart_putc(*msg++);
    }

    while (1) {
        /* Permanent fatal halt. */
    }
}
/*---------------------------------------------------------------------------*/
