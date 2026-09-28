/* SPDX-License-Identifier: MIT */
#include "builtin_files.h"
#include "homecore/vfs/vfs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static int file_error(const char *command, const char *path) {
    printf("%s: %s: %s\n", command, path, strerror(errno));
    return 1;
}

static int print_entry(const char *name, bool directory, void *context) {
    (void)context;
    return printf("%s%s\n", name, directory ? "/" : "") < 0 ? -1 : 0;
}

int shell_builtin_ls(shell_context_t *context, int argc, char **argv) {
    if (argc > 2) {
        puts("Usage: ls [path]");
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : ".";
    char resolved[VFS_PATH_CAPACITY];
    if (vfs_resolve_path(context->cwd, path, resolved) < 0) return file_error("ls", path);
    vfs_node_t *node = vfs_find_node(resolved);
    if (!node) return file_error("ls", path);
    if (!node->is_directory) {
        return print_entry(strrchr(node->name, '/') + 1, false, NULL);
    }
    if (vfs_list(resolved, print_entry, NULL) < 0) return file_error("ls", path);
    return 0;
}

int shell_builtin_mkdir(shell_context_t *context, int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: mkdir path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        char resolved[VFS_PATH_CAPACITY];
        if (vfs_resolve_path(context->cwd, argv[i], resolved) < 0 ||
            vfs_mkdir(resolved) < 0) status = file_error("mkdir", argv[i]);
    }
    return status;
}

int shell_builtin_cat(shell_context_t *context, int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: cat path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        char resolved[VFS_PATH_CAPACITY];
        if (vfs_resolve_path(context->cwd, argv[i], resolved) < 0) {
            status = file_error("cat", argv[i]);
            continue;
        }
        int fd = vfs_open(resolved, O_RDONLY);
        if (fd < 0) {
            status = file_error("cat", argv[i]);
            continue;
        }
        char buffer[128];
        int length;
        while ((length = vfs_read(fd, buffer, sizeof(buffer))) > 0) {
            if (fwrite(buffer, 1, (size_t)length, stdout) != (size_t)length) {
                status = file_error("cat", argv[i]);
                break;
            }
        }
        if (length < 0) status = file_error("cat", argv[i]);
        if (vfs_close(fd) < 0) status = file_error("cat", argv[i]);
    }
    return status;
}
