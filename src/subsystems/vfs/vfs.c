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
#include <string.h>
#include "homecore/autoconf.h"
/*---------------------------------------------------------------------------*/
#include "homecore/vfs/vfs.h"
/*---------------------------------------------------------------------------*/
static vfs_node_t *vfs_root = NULL;
/*---------------------------------------------------------------------------*/
static vfs_node_t *vfs_fd_table[CONFIG_HOMECORE_VFS_MAX_OPEN_FILES] = {0};
/*---------------------------------------------------------------------------*/
void vfs_init(void) {
}
/*---------------------------------------------------------------------------*/
void vfs_register_node(vfs_node_t *node) {
    if (!node || !node->name) {
        return;
    }

    node->next = vfs_root;
    vfs_root = node;
}
/*---------------------------------------------------------------------------*/
vfs_node_t *vfs_find_node(const char *name) {
    vfs_node_t *current = NULL;

    for (current = vfs_root; current != NULL; current = current->next) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
    }

    return NULL;
}
/*---------------------------------------------------------------------------*/
int vfs_open(const char *name, int flags) {
    vfs_node_t *node = vfs_find_node(name);
    if (!node) {
        return -1;
    }

    int fd = -1;
    for (int i = 0; i < CONFIG_HOMECORE_VFS_MAX_OPEN_FILES; ++i) {
        if (!vfs_fd_table[i]) {
            vfs_fd_table[i] = node;
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        return -2; // No available file descriptor
    }

    if (node->ops.open) {
        int result = node->ops.open(node, flags);
        if (result < 0) {
            vfs_fd_table[fd] = NULL;
            return result; // Return the error code from the open operation
        }
    }

    return fd;
}
/*---------------------------------------------------------------------------*/
int vfs_close(int fd) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        return -1;
    }

    if (vfs_fd_table[fd] == NULL) {
        return -2; // File descriptor not open
    }

    int result = 0;
    if (vfs_fd_table[fd]->ops.close) {
        result = vfs_fd_table[fd]->ops.close(vfs_fd_table[fd]);
    }

    vfs_fd_table[fd] = NULL;
    return result;
}
/*---------------------------------------------------------------------------*/
int vfs_read(int fd, void *buf, unsigned len) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        return -1;
    }

    if (vfs_fd_table[fd] == NULL) {
        return -2; // File descriptor not open
    }

    if (vfs_fd_table[fd]->ops.read) {
        return vfs_fd_table[fd]->ops.read(vfs_fd_table[fd], buf, len);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_write(int fd, const void *buf, unsigned len) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        return -1;
    }

    if (vfs_fd_table[fd] == NULL) {
        return -2; // File descriptor not open
    }

    if (vfs_fd_table[fd]->ops.write) {
        return vfs_fd_table[fd]->ops.write(vfs_fd_table[fd], buf, len);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_ioctl(int fd, unsigned request, void *arg) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        return -1;
    }

    if (vfs_fd_table[fd] == NULL) {
        return -2; // File descriptor not open
    }

    if (vfs_fd_table[fd]->ops.ioctl) {
        return vfs_fd_table[fd]->ops.ioctl(vfs_fd_table[fd], request, arg);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        return -1;
    }

    if (vfs_fd_table[fd] == NULL) {
        return -2; // File descriptor not open
    }

    if (vfs_fd_table[fd]->ops.lseek) {
        return vfs_fd_table[fd]->ops.lseek(vfs_fd_table[fd], offset, whence);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
