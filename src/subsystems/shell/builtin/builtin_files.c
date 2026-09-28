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

int shell_builtin_ls(int argc, char **argv) {
    if (argc > 2) {
        puts("Usage: ls [path]");
        return 1;
    }
    const char *path = argc == 2 ? argv[1] : "/";
    vfs_node_t *node = vfs_find_node(path);
    if (!node) return file_error("ls", path);
    if (!node->is_directory) {
        return print_entry(strrchr(node->name, '/') + 1, false, NULL);
    }
    if (vfs_list(path, print_entry, NULL) < 0) return file_error("ls", path);
    return 0;
}

int shell_builtin_mkdir(int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: mkdir path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        if (vfs_mkdir(argv[i]) < 0) status = file_error("mkdir", argv[i]);
    }
    return status;
}

int shell_builtin_cat(int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: cat path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        int fd = vfs_open(argv[i], O_RDONLY);
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
