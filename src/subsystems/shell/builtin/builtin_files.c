/* SPDX-License-Identifier: MIT */
#include "builtin_files.h"
#include "homecore/vfs/vfs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

/* Longest text the write command sends, including the newline. */
#define SHELL_WRITE_CAPACITY 128U

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
    if (vfs_resolve_path(context->cwd, path, resolved) < 0) {
        return file_error("ls", path);
    }
    vfs_stat_t stat;
    if (vfs_stat(resolved, &stat) < 0) {
        return file_error("ls", path);
    }
    if (!stat.is_directory) {
        return print_entry(strrchr(resolved, '/') + 1, false, NULL);
    }
    if (vfs_list(resolved, print_entry, NULL) < 0) {
        return file_error("ls", path);
    }
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
        if (vfs_resolve_path(context->cwd, argv[i], resolved) < 0 || vfs_mkdir(resolved) < 0) {
            status = file_error("mkdir", argv[i]);
        }
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
        if (length < 0) {
            status = file_error("cat", argv[i]);
        }
        if (vfs_close(fd) < 0) {
            status = file_error("cat", argv[i]);
        }
    }
    return status;
}

int shell_builtin_rmdir(shell_context_t *context, int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: rmdir path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        char path[VFS_PATH_CAPACITY];
        if (vfs_resolve_path(context->cwd, argv[i], path) < 0) {
            status = file_error("rmdir", argv[i]);
            continue;
        }
        if (strcmp(path, context->cwd) == 0) {
            errno = EBUSY;
            status = file_error("rmdir", argv[i]);
        } else if (vfs_rmdir(path) < 0) {
            status = file_error("rmdir", argv[i]);
        }
    }
    return status;
}

int shell_builtin_touch(shell_context_t *context, int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: touch path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        char path[VFS_PATH_CAPACITY];
        size_t length = strlen(argv[i]);
        if (length && argv[i][length - 1] == '/') {
            errno = EISDIR;
            status = file_error("touch", argv[i]);
            continue;
        }
        if (vfs_resolve_path(context->cwd, argv[i], path) < 0) {
            status = file_error("touch", argv[i]);
            continue;
        }
        vfs_stat_t stat;
        if (vfs_stat(path, &stat) == 0) {
            if (!stat.is_regular) {
                errno = stat.is_directory ? EISDIR : EPERM;
                status = file_error("touch", argv[i]);
            }
            continue; /* No timestamps yet; preserve existing contents. */
        }
        int fd = vfs_open(path, O_CREAT | O_WRONLY);
        if (fd < 0) {
            status = file_error("touch", argv[i]);
        } else if (vfs_close(fd) < 0) {
            status = file_error("touch", argv[i]);
        }
    }
    return status;
}

int shell_builtin_rm(shell_context_t *context, int argc, char **argv) {
    if (argc < 2) {
        puts("Usage: rm path...");
        return 1;
    }
    int status = 0;
    for (int i = 1; i < argc; i++) {
        char path[VFS_PATH_CAPACITY];
        if (vfs_resolve_path(context->cwd, argv[i], path) < 0 || vfs_unlink(path) < 0) {
            status = file_error("rm", argv[i]);
        }
    }
    return status;
}

/* Copy every byte of source to target; returns 0 or -1 with errno set. */
static int copy_file(int source, int target) {
    char buffer[128];
    int length;
    while ((length = vfs_read(source, buffer, sizeof(buffer))) > 0) {
        int offset = 0;
        while (offset < length) {
            int written = vfs_write(target, buffer + offset, (unsigned)(length - offset));
            if (written <= 0) {
                if (written == 0) {
                    errno = EIO;
                }
                return -1;
            }
            offset += written;
        }
    }
    return length < 0 ? -1 : 0;
}

int shell_builtin_cp(shell_context_t *context, int argc, char **argv) {
    if (argc != 3) {
        puts("Usage: cp source target");
        return 1;
    }
    char source[VFS_PATH_CAPACITY];
    char target[VFS_PATH_CAPACITY];
    if (vfs_resolve_path(context->cwd, argv[1], source) < 0) {
        return file_error("cp", argv[1]);
    }
    if (vfs_resolve_path(context->cwd, argv[2], target) < 0) {
        return file_error("cp", argv[2]);
    }
    vfs_stat_t stat;
    if (vfs_stat(target, &stat) == 0 && stat.is_directory) {
        /* Copy into the directory under the source's name. */
        const char *name = strrchr(source, '/') + 1;
        size_t length = strlen(target);
        const char *separator = target[length - 1] == '/' ? "" : "/";
        int written = snprintf(target + length, sizeof(target) - length, "%s%s", separator, name);
        if (written < 0 || (size_t)written >= sizeof(target) - length) {
            errno = ENAMETOOLONG;
            return file_error("cp", argv[2]);
        }
    }
    if (strcmp(source, target) == 0) {
        errno = EINVAL;
        return file_error("cp", argv[2]);
    }
    int input = vfs_open(source, O_RDONLY);
    if (input < 0) {
        return file_error("cp", argv[1]);
    }
    int status = 0;
    int output = vfs_open(target, O_WRONLY | O_CREAT | O_TRUNC);
    if (output < 0) {
        status = file_error("cp", argv[2]);
    } else {
        if (copy_file(input, output) < 0) {
            status = file_error("cp", argv[2]);
        }
        if (vfs_close(output) < 0) {
            status = file_error("cp", argv[2]);
        }
    }
    (void)vfs_close(input);
    return status;
}

/* Write all of text; returns 0 or -1 with errno set. */
static int write_all(int fd, const char *text, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        int written = vfs_write(fd, text + offset, (unsigned)(length - offset));
        if (written <= 0) {
            if (written == 0) {
                errno = EIO;
            }
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

int shell_builtin_write(shell_context_t *context, int argc, char **argv) {
    if (argc < 3) {
        puts("Usage: write path text...");
        return 1;
    }
    /* The words joined by spaces, with a trailing newline, as one write, so
     * a device such as /dev/led0 receives the whole command at once. */
    char text[SHELL_WRITE_CAPACITY];
    size_t length = 0;
    for (int i = 2; i < argc; i++) {
        size_t size = strlen(argv[i]);
        if (length + size + 1U >= sizeof(text)) {
            errno = E2BIG;
            return file_error("write", argv[1]);
        }
        memcpy(text + length, argv[i], size);
        length += size;
        text[length++] = i + 1 < argc ? ' ' : '\n';
    }
    char path[VFS_PATH_CAPACITY];
    if (vfs_resolve_path(context->cwd, argv[1], path) < 0) {
        return file_error("write", argv[1]);
    }
    int fd = vfs_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        return file_error("write", argv[1]);
    }
    int status = 0;
    if (write_all(fd, text, length) < 0) {
        status = file_error("write", argv[1]);
    }
    if (vfs_close(fd) < 0) {
        status = file_error("write", argv[1]);
    }
    return status;
}
