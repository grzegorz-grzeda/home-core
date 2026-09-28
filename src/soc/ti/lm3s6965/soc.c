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
#include "homecore/soc/soc.h"
#include "homecore/vfs/vfs.h"
#include <errno.h>
#include <limits.h>
#include "soc_cmsis.h"
/*---------------------------------------------------------------------------*/
#define UART_FR_TXFF (1u << 5)
#define UART_FR_RXFE (1u << 4)
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static int soc_uart_read(vfs_node_t *node, void *buf, unsigned len) {
    if (len == 0) {
        return 0;
    }

    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    UART0_Type *uart = (UART0_Type *)node->driver_data;
    while (uart->FR & UART_FR_RXFE) {
        /* Blocking console read: wait for a received byte. */
    }
    char *c = (char *)buf;

    *c = (char)(uart->DR & 0xff);

    return 1; // Return the number of bytes read
}
/*---------------------------------------------------------------------------*/
static int soc_uart_write(vfs_node_t *node, const void *buf, unsigned len) {
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
    UART0_Type *uart = (UART0_Type *)node->driver_data;
    const char *c = (const char *)buf;

    unsigned cnt = 0;
    while (len-- > 0) {
        while (uart->FR & UART_FR_TXFF) {
            /* Poll until the transmit FIFO has space. */
        }

        uart->DR = (uint8_t)(*c++);
        cnt++;
    }

    return (int)cnt;
}
/*---------------------------------------------------------------------------*/
static vfs_node_t uart0_node = {.name = "/dev/uart0",
                                .ops = {.open = NULL,
                                        .close = NULL,
                                        .read = soc_uart_read,
                                        .write = soc_uart_write,
                                        .ioctl = NULL,
                                        .lseek = NULL},
                                .driver_data = UART0,
                                .next = NULL};
static vfs_node_t uart1_node = {.name = "/dev/uart1",
                                .ops = {.open = NULL,
                                        .close = NULL,
                                        .read = soc_uart_read,
                                        .write = soc_uart_write,
                                        .ioctl = NULL,
                                        .lseek = NULL},
                                .driver_data = UART1,
                                .next = NULL};
static vfs_node_t uart2_node = {.name = "/dev/uart2",
                                .ops = {.open = NULL,
                                        .close = NULL,
                                        .read = soc_uart_read,
                                        .write = soc_uart_write,
                                        .ioctl = NULL,
                                        .lseek = NULL},
                                .driver_data = UART2,
                                .next = NULL};
/*---------------------------------------------------------------------------*/
void soc_init(void) {
    vfs_register_node(&uart0_node);
    vfs_register_node(&uart1_node);
    vfs_register_node(&uart2_node);
}
/*---------------------------------------------------------------------------*/
