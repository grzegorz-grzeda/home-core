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
#include <stdint.h>
#include <stddef.h>
#include "homecore/shell/shell.h"
/*---------------------------------------------------------------------------*/
typedef struct shell_command shell_command_t;
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef struct shell_command {
    const char *name;
    const char *help;
    shell_command_handler_t handler;
    shell_command_t *next;
} shell_command_t;
/*---------------------------------------------------------------------------*/
static shell_command_t *shell_commands = NULL;
static shell_send_char_t shell_send_char = NULL;
/*---------------------------------------------------------------------------*/
void shell_init(shell_send_char_t send_char) {
    shell_send_char = send_char;
}
/*---------------------------------------------------------------------------*/
void shell_register_command(const char *name, const char *help, shell_command_handler_t handler) {
    if (!name || !help || !handler) {
        return;
    }
    shell_command_t *command = (shell_command_t *)calloc(1, sizeof(shell_command_t));
    if (!command) {
        return;
    }
    command->name = name;
    command->help = help;
    command->handler = handler;
    command->next = shell_commands;
    shell_commands = command;
}
/*---------------------------------------------------------------------------*/
void shell_process_char(char c) {
}
/*---------------------------------------------------------------------------*/
