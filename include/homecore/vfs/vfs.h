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
#ifndef HOME_CORE_VFS_H
#define HOME_CORE_VFS_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
typedef struct vfs_node vfs_node_t;
/*---------------------------------------------------------------------------*/
typedef struct {
    int (*open)(vfs_node_t *node, int flags);
    int (*close)(vfs_node_t *node);

    int (*read)(vfs_node_t *node, void *buf, unsigned len);
    int (*write)(vfs_node_t *node, const void *buf, unsigned len);

    int (*ioctl)(vfs_node_t *node, unsigned request, void *arg);

    int (*lseek)(vfs_node_t *node, int offset, int whence);
} vfs_ops_t;
/*---------------------------------------------------------------------------*/
typedef struct vfs_node {
    const char *name;
    vfs_ops_t ops;
    void *driver_data;
    vfs_node_t *next;
} vfs_node_t;
/*---------------------------------------------------------------------------*/
void vfs_init(void);
void vfs_register_node(vfs_node_t *node);
vfs_node_t *vfs_find_node(const char *name);
int vfs_open(const char *name, int flags);
int vfs_close(int fd);
int vfs_read(int fd, void *buf, unsigned len);
int vfs_write(int fd, const void *buf, unsigned len);
int vfs_ioctl(int fd, unsigned request, void *arg);
int vfs_lseek(int fd, int offset, int whence);
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_VFS_H
/*---------------------------------------------------------------------------*/
