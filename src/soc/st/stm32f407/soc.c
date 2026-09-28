// SPDX-License-Identifier: MIT
#include "homecore/soc/soc.h"
#include "homecore/board/board.h"
#include "homecore/vfs/vfs.h"

static int uart_read(vfs_node_t *node, void *buf, unsigned len) {
    (void)node;
    if (len == 0) return 0;
    if (!buf) return -1;
    *(char *)buf = (char)board_uart_getc();
    return 1;
}

static int uart_write(vfs_node_t *node, const void *buf, unsigned len) {
    (void)node;
    if (len == 0) return 0;
    if (!buf) return -1;
    const char *bytes = buf;
    for (unsigned i = 0; i < len; ++i) board_uart_putc(bytes[i]);
    return (int)len;
}

static vfs_node_t console = {
    .name = "/dev/uart0",
    .ops = {.read = uart_read, .write = uart_write},
};

void soc_init(void) {
    vfs_register_node(&console);
}
