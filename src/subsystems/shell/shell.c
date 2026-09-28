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
#include "builtin_basic.h"
#include "builtin_files.h"
#include "builtin_session.h"
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
static int shell_print_help(shell_context_t *context, int argc, char **argv) {
    (void)context;
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
    if (shell_commands) return;
    shell_register_command("cd", "Change working directory: cd [path]", shell_builtin_cd);
    shell_register_command("pwd", "Print working directory", shell_builtin_pwd);
    shell_register_command("whoami", "Print current user", shell_builtin_whoami);
    shell_register_command("id", "Print user and group IDs", shell_builtin_id);
    shell_register_command("help", "Display this help message", shell_print_help);
    shell_register_command("ls", "List directory entries: ls [path]", shell_builtin_ls);
    shell_register_command("mkdir", "Create RAM directories: mkdir path...", shell_builtin_mkdir);
    shell_register_command("cat", "Print file contents: cat path...", shell_builtin_cat);
    shell_register_command("basic", "BASIC language interpreter", shell_builtin_basic);
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
int shell_read_line(char *buffer, size_t max_length) {
    static int skip_lf = 0;
    size_t len = 0;

    if (buffer == NULL || max_length == 0) {
        return -1;
    }

    buffer[0] = '\0';
    fflush(stdout);
    while (1) {
        int ch = getchar();

        if (ch == EOF) {
            return -1;
        }
        if (skip_lf) {
            skip_lf = 0;
            if (ch == '\n') {
                continue;
            }
        }

        /*
         * Enter can come as:
         * '\r'     CR
         * '\n'     LF
         * "\r\n"   CRLF
         */
        if (ch == '\r' || ch == '\n') {
            skip_lf = (ch == '\r');
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
                fflush(stdout);
            }

            continue;
        }

        /*
         * Ctrl+C / Ctrl+D / Ctrl+Z
         */
        if (ch == 0x03 || ch == 0x04 || ch == 0x1A) {
            printf("^%c\r\n", ch + '@');
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
                fflush(stdout);
            } else {
                putchar('\a');
            }

            continue;
        }
    }
}
/*---------------------------------------------------------------------------*/
int shell_execute_line(shell_context_t *context, char *line) {
    if (!context || !context->user || !line) return -1;
    char *args[CONFIG_HOMECORE_SHELL_MAX_ARGS + 1];
    size_t argc = 0;
    char *save = NULL;
    char *token = strtok_r(line, " \t", &save);
    while (token) {
        if (argc == CONFIG_HOMECORE_SHELL_MAX_ARGS) {
            puts("Too many arguments");
            context->last_status = 2;
            return 2;
        }
        args[argc++] = token;
        token = strtok_r(NULL, " \t", &save);
    }
    args[argc] = NULL;
    if (!argc) return context->last_status;

    for (shell_command_t *command = shell_commands; command; command = command->next) {
        if (strcmp(command->name, args[0]) == 0) {
            session_t *previous = session_set_current(context);
            int status = command->handler(context, (int)argc, args);
            session_set_current(previous);
            context->last_status = status;
            return status;
        }
    }
    printf("Unknown command: %s\n", args[0]);
    context->last_status = 127;
    return 127;
}
/*---------------------------------------------------------------------------*/
void shell_run(void) {
    while (1) {
        shell_context_t *context = session_current();
        printf("%s:%s%s", context->user->name, context->cwd, CONFIG_HOMECORE_SHELL_PROMPT);
        int len = shell_read_line(shell_input_buffer, sizeof(shell_input_buffer));
        if (len > 0) {
            shell_execute_line(context, shell_input_buffer);
        }
    }
}
/*---------------------------------------------------------------------------*/
