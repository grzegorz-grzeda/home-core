/* SPDX-License-Identifier: MIT */
#include "builtin_session.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

int shell_builtin_cd(shell_context_t *context, int argc, char **argv) {
    if (argc > 2) {
        puts("Usage: cd [path]");
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : context->user->home;
    if (session_chdir(context, path) < 0) {
        printf("cd: %s: %s\n", path, strerror(errno));
        return 1;
    }
    return 0;
}

int shell_builtin_pwd(shell_context_t *context, int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        puts("Usage: pwd");
        return 1;
    }
    puts(context->cwd);
    return 0;
}

int shell_builtin_whoami(shell_context_t *context, int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        puts("Usage: whoami");
        return 1;
    }
    puts(context->user->name);
    return 0;
}

int shell_builtin_id(shell_context_t *context, int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        puts("Usage: id");
        return 1;
    }
    printf("uid=%lu(%s) gid=%lu\n",
           (unsigned long)context->user->uid,
           context->user->name,
           (unsigned long)context->user->gid);
    return 0;
}
