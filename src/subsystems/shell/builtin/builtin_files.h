/* SPDX-License-Identifier: MIT */
#ifndef HOMECORE_BUILTIN_FILES_H
#define HOMECORE_BUILTIN_FILES_H
#include "homecore/shell/shell.h"

int shell_builtin_ls(shell_context_t *context, int argc, char **argv);
int shell_builtin_mkdir(shell_context_t *context, int argc, char **argv);
int shell_builtin_cat(shell_context_t *context, int argc, char **argv);

#endif
