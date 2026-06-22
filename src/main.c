#include "board/board.h"

static void puts_uart(const char *s)
{
    while (*s) {
        if (*s == '\n') {
            board_uart_putc('\r');
        }

        board_uart_putc(*s++);
    }
}

int main(void)
{
    board_init();

    puts_uart("\nHomeCore Monitor\n");
    puts_uart("target: qemu_lm3s6965evb\n");
    puts_uart("> ");

    while (1) {
        int c = board_uart_getc();

        board_uart_putc((char)c);

        if (c == '\r' || c == '\n') {
            puts_uart("\n> ");
        }
    }
}