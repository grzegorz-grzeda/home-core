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
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stddef.h>
/*---------------------------------------------------------------------------*/
#include "board/board.h"
/*---------------------------------------------------------------------------*/
extern uint8_t _heap_start;
extern uint8_t _heap_end;
/*---------------------------------------------------------------------------*/
static uint8_t *heap_current = &_heap_start;
/*---------------------------------------------------------------------------*/
void *_sbrk(ptrdiff_t incr)
{
    (void)incr;
    uint8_t *prev_heap = heap_current;
    uint8_t *next_heap = heap_current + incr;

    if (next_heap > &_heap_end) {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_current = next_heap;
    return prev_heap;
}
/*---------------------------------------------------------------------------*/
int _write(int fd, const char *buf, int len)
{
    (void)fd;

    for (int i = 0; i < len; i++) {
        if (buf[i] == '\n') {
            board_uart_putc('\r');
        }

        board_uart_putc(buf[i]);
    }

    return len;
}
/*---------------------------------------------------------------------------*/
int _read(int fd, char *buf, int len)
{
    (void)fd;

    for (int i = 0; i < len; i++) {
        int c = board_uart_getc();

        if (c == '\r') {
            c = '\n';
        }

        buf[i] = (char)c;
        board_uart_putc((char)c);

        if (c == '\n') {
            return i + 1;
        }
    }

    return len;
}
/*---------------------------------------------------------------------------*/
int _close(int fd)
{
    (void)fd;
    return -1;
}
/*---------------------------------------------------------------------------*/
int _fstat(int fd, struct stat *st)
{
    (void)fd;

    st->st_mode = S_IFCHR;
    return 0;
}
/*---------------------------------------------------------------------------*/
int _isatty(int fd)
{
    (void)fd;
    return 1;
}
/*---------------------------------------------------------------------------*/
int _lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;

    return 0;
}
/*---------------------------------------------------------------------------*/
int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;

    errno = EINVAL;
    return -1;
}
/*---------------------------------------------------------------------------*/
int _getpid(void)
{
    return 1;
}