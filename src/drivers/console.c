// SPDX-License-Identifier: MIT
/* The board interface's polling console, implemented once on top of the
 * driver chosen by the device description (chosen.console). */
#include "homecore/drivers/console.h"
#include "homecore/board/board.h"

bool console_ready(void) {
    return dt_console.ops->ready(dt_console.device);
}

void console_flush(void) {
    dt_console.ops->flush(dt_console.device);
}

void board_uart_putc(char c) {
    dt_console.ops->putc(dt_console.device, c);
}

int board_uart_getc(void) {
    return dt_console.ops->getc(dt_console.device);
}

int board_uart_has_data(void) {
    return dt_console.ops->has_data(dt_console.device);
}
