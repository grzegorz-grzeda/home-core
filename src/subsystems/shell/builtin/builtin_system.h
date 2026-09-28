/* SPDX-License-Identifier: MIT */
#ifndef HOMECORE_BUILTIN_SYSTEM_H
#define HOMECORE_BUILTIN_SYSTEM_H
#include "homecore/shell/shell.h"
int shell_builtin_mem(shell_context_t *context, int argc, char **argv);
int shell_builtin_uptime(shell_context_t *context, int argc, char **argv);
int shell_builtin_clear(shell_context_t *context, int argc, char **argv);
int shell_builtin_reboot(shell_context_t *context, int argc, char **argv);
#endif
