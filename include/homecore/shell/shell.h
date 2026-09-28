/*
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
/**
 * @file
 * @brief Command shell: line input, command registration, and dispatch.
 */
/*---------------------------------------------------------------------------*/
#ifndef HOME_CORE_SHELL_H
#define HOME_CORE_SHELL_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
#include "homecore/session/session.h"
/*---------------------------------------------------------------------------*/
/**
 * @defgroup shell Shell
 * @ingroup subsystems
 * @brief Interactive console shell and command registry.
 *
 * The shell runs in the foreground on the standard streams. Built-in commands
 * live in `src/subsystems/shell/builtin/`. The shell and extending guides
 * listed under @ref mainpage_guides describe the commands and how to add one.
 * @{
 */
/*---------------------------------------------------------------------------*/
/** @brief Command execution context: the session the command runs in. */
typedef session_t shell_context_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Command handler.
 *
 * During the call, @p context is the current session, so libc path
 * operations resolve against its working directory.
 *
 * @param context Session running the command.
 * @param argc    Number of arguments, including the command name.
 * @param argv    Arguments; `argv[argc]` is `NULL`. The strings point into
 *                the input line and are valid only during the call.
 *
 * @return Exit status stored in the session's `last_status`; 0 for success.
 */
typedef int (*shell_command_handler_t)(shell_context_t *context, int argc, char **argv);
/*---------------------------------------------------------------------------*/
/**
 * @brief Register the built-in commands.
 *
 * Does nothing if any command is already registered. Call it before
 * registering other commands, or the built-ins are never added.
 */
void shell_init(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Add a command.
 *
 * The strings are retained by pointer and must outlive the shell. Each
 * registration allocates a list entry from the heap. On allocation failure or
 * a `NULL` argument, the command is silently not registered. A later
 * registration with the same name hides the earlier one, and `help` lists the
 * newest commands first.
 *
 * @param name    Command name matched against the first input token.
 * @param help    One-line description shown by `help`, including usage.
 * @param handler Function run for the command.
 */
void shell_register_command(const char *name, const char *help, shell_command_handler_t handler);
/*---------------------------------------------------------------------------*/
/**
 * @brief Read and echo one console line.
 *
 * Accepts CR, LF, or CRLF as one line ending. Handles backspace and delete,
 * clears the screen on Ctrl-L, rings the bell when the buffer is full, and
 * ignores other control characters.
 *
 * @param[out] buffer     Destination for the NUL-terminated line.
 * @param      max_length Capacity of @p buffer, including the terminator.
 *
 * @return Line length without the terminator; -1 for invalid arguments or end
 *         of file; -2 for Ctrl-C, Ctrl-D, or Ctrl-Z, which empties @p buffer.
 */
int shell_read_line(char *buffer, size_t max_length);
/*---------------------------------------------------------------------------*/
/**
 * @brief Tokenize and run one command line in a session.
 *
 * Splits @p line on spaces and tabs in place; there is no quoting or
 * escaping. Makes @p context the current session while the command runs, then
 * restores the previous one.
 *
 * @param context Session that runs the command. Its `user` must not be `NULL`.
 * @param line    Writable NUL-terminated input. It is modified.
 *
 * @return The command's status; 2 for more than
 *         `CONFIG_HOMECORE_SHELL_MAX_ARGS` arguments; 127 for an unknown
 *         command; the previous status for an empty line. Each of these is
 *         stored in `last_status`. Returns -1 without changing the session if
 *         an argument or `context->user` is `NULL`.
 */
int shell_execute_line(shell_context_t *context, char *line);
/*---------------------------------------------------------------------------*/
/**
 * @brief Run the interactive shell. Does not return.
 *
 * Repeatedly prints the prompt for the current session, reads a line with
 * shell_read_line(), and runs it with shell_execute_line().
 */
void shell_run(void);
/*---------------------------------------------------------------------------*/
/** @} */
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_SHELL_H
/*---------------------------------------------------------------------------*/
