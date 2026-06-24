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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "homecore/shell/shell.h"
#include "homecore/autoconf.h"
/*---------------------------------------------------------------------------*/
#define SHELL_TERMINATOR '\n'
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
static char shell_input_buffer[CONFIG_HOMECORE_SHELL_MAX_INPUT_LENGTH];
/*---------------------------------------------------------------------------*/
static int shell_print_help(int argc, char **argv) {
    (void)argc;
    (void)argv;
    shell_command_t *command = shell_commands;
    printf("Available commands:\n");
    while (command != NULL) {
        printf("  %s: %s\n", command->name, command->help);
        command = command->next;
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
void shell_init(void) {
    shell_register_command("help", "Display this help message", shell_print_help);
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
static int shell_read_line(char *buffer, size_t max_length) {
    size_t len = 0;

    if (buffer == NULL || max_length == 0) {
        return -1;
    }

    while (1) {
        int ch = getchar();

        if (ch < 0) {
            continue;
        }

        /*
         * Enter can come as:
         * '\r'     CR
         * '\n'     LF
         * "\r\n"   CRLF
         */
        if (ch == '\r' || ch == '\n') {
            putchar('\r');
            putchar('\n');

            buffer[len] = '\0';
            return (int)len;
        }

        /*
         * Backspace in terminal can be sent as either:
         * 0x08 = BS
         * 0x7F = DEL
         */
        if (ch == '\b' || ch == 0x7F) {
            if (len > 0) {
                len--;
                printf("\b \b");
            }

            continue;
        }

        /*
         * Ctrl+C
         */
        if (ch == 0x03) {
            printf("^C\r\n");
            buffer[0] = '\0';
            return -2;
        }

        /*
         * Ctrl+L - clear screen
         */
        if (ch == 0x0C) {
            printf("\x1b[2J\x1b[H");
            continue;
        }

        if (ch >= 32 && ch <= 126) {
            if (len < max_length - 1) {
                buffer[len++] = (char)ch;
                putchar((char)ch); // echo
            } else {
                putchar('\a');
            }

            continue;
        }
    }
}
/*---------------------------------------------------------------------------*/
static void shell_execute(const char *line, size_t len) {
    if (len == 0) {
        return;
    }

    char *args[CONFIG_HOMECORE_SHELL_MAX_ARGS];
    size_t argc = 0;

    char *token = strtok((char *)line, " ");
    while (token != NULL && argc < CONFIG_HOMECORE_SHELL_MAX_ARGS) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }

    if (argc == 0) {
        return;
    }

    shell_command_t *command = shell_commands;
    while (command != NULL) {
        if (strcmp(command->name, args[0]) == 0) {
            command->handler((int)argc, args);
            return;
        }
        command = command->next;
    }

    printf("Unknown command: %s\n", args[0]);
}
/*---------------------------------------------------------------------------*/
void shell_run(void) {
    while (1) {
        printf("%s", CONFIG_HOMECORE_SHELL_PROMPT);
        size_t len = shell_read_line(shell_input_buffer, sizeof(shell_input_buffer));
        shell_execute(shell_input_buffer, len);
    }
}
/*---------------------------------------------------------------------------*/
