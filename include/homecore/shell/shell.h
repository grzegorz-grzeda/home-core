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
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
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
#ifndef HOME_CORE_SHELL_H
#define HOME_CORE_SHELL_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef void (*shell_send_char_t)(char c);
typedef int (*shell_command_handler_t)(int argc, char **argv);
/*---------------------------------------------------------------------------*/
void shell_init(shell_send_char_t send_char);
/*---------------------------------------------------------------------------*/
void shell_register_command(const char *name, const char *help, shell_command_handler_t handler);
/*---------------------------------------------------------------------------*/
void shell_process_char(char c);
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_SHELL_H
/*---------------------------------------------------------------------------*/
