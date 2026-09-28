// SPDX-License-Identifier: MIT
/* Shared soc_init() for STM32 SoCs: the console is /dev/uart0, driven by the
 * board's polling UART functions, so this file has no chip-specific code. */
#include "homecore/soc/soc.h"
#include "homecore/board/board.h"
#include "homecore/vfs/vfs.h"
#include <errno.h>
#include <limits.h>

static int uart_read(vfs_node_t *node, void *buf, unsigned len) {
    (void)node;
    if (len == 0) {
        return 0;
    }
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    *(char *)buf = (char)board_uart_getc();
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

    (void)node;
    if (!buf) {
        errno = EFAULT;
        return -1;
    }
    const char *bytes = buf;
    for (unsigned i = 0; i < len; ++i) {
        board_uart_putc(bytes[i]);
    }
    return (int)len;
}

static vfs_node_t console = {
    .name = "/dev/uart0",
    .ops = {.read = uart_read, .write = uart_write},
};

void soc_init(void) {
    vfs_register_node(&console);
}
