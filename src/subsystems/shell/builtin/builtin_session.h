/* SPDX-License-Identifier: MIT */
#ifndef HOMECORE_BUILTIN_SESSION_H
#define HOMECORE_BUILTIN_SESSION_H
#include "homecore/shell/shell.h"
int shell_builtin_cd(shell_context_t *context, int argc, char **argv);
int shell_builtin_pwd(shell_context_t *context, int argc, char **argv);
int shell_builtin_whoami(shell_context_t *context, int argc, char **argv);
int shell_builtin_id(shell_context_t *context, int argc, char **argv);
#endif
