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
#include "homecore/vfs/vfs.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "homecore/autoconf.h"
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
typedef struct vfs_mount vfs_mount_t;
/* Allocated by vfs_open() and freed by vfs_close(). Descriptors on snapshot
 * nodes carry VFS_SNAPSHOT_CAPACITY bytes of snapshot storage; others none. */
typedef struct vfs_file_descriptor {
    vfs_node_t *node;   /* NULL for files on a mounted filesystem */
    vfs_mount_t *mount; /* Set for files on a mounted filesystem */
    void *handle;       /* The mounted filesystem's file handle */
    int flags;
    unsigned snapshot_length;
    unsigned position;
    char snapshot[];
} vfs_file_descriptor_t;
/*---------------------------------------------------------------------------*/
/* A RAM directory or file with its canonical path, allocated in one block and
 * freed when removed. node must stay the first member: remove_entry() converts. */
typedef struct vfs_entry {
    vfs_node_t node;
    char path[];
} vfs_entry_t;
/*---------------------------------------------------------------------------*/
static vfs_node_t dev_directory = {.name = "/dev", .is_directory = true};
static vfs_node_t root_directory = {.name = "/", .is_directory = true, .next = &dev_directory};
static vfs_node_t *vfs_root = &root_directory;
/* A mounted filesystem: its mount-point node and path, allocated in one block. */
struct vfs_mount {
    vfs_node_t node;
    const vfs_fs_ops_t *ops;
    void *fs;
    char path[];
};
/* Live entries, capped by the Kconfig maxima. */
static unsigned directory_count;
static unsigned ram_file_count;
static vfs_mount_t *mounts[CONFIG_HOMECORE_VFS_MAX_MOUNTS];
/*---------------------------------------------------------------------------*/
static vfs_file_descriptor_t *vfs_fd_table[CONFIG_HOMECORE_VFS_MAX_OPEN_FILES];
/*---------------------------------------------------------------------------*/
void vfs_init(void) {
}
/*---------------------------------------------------------------------------*/
void vfs_register_node(vfs_node_t *node) {
    if (!node || !node->name) {
        return;
    }

    for (vfs_node_t *existing = vfs_root; existing; existing = existing->next) {
        if (strcmp(existing->name, node->name) == 0) {
            return;
        }
    }
    node->next = vfs_root;
    vfs_root = node;
}
/*---------------------------------------------------------------------------*/
static vfs_node_t *find_exact(const char *path) {
    for (vfs_node_t *node = vfs_root; node; node = node->next) {
        if (strcmp(node->name, path) == 0) {
            return node;
        }
    }
    return NULL;
}

/* The mount containing a canonical path, or NULL. *relative receives the part
 * below the mount point, or "/" for the mount point itself. */
static vfs_mount_t *find_mount(const char *path, const char **relative) {
    for (unsigned i = 0; i < CONFIG_HOMECORE_VFS_MAX_MOUNTS; i++) {
        vfs_mount_t *mount = mounts[i];
        if (!mount) {
            continue;
        }
        size_t length = strlen(mount->path);
        if (strncmp(path, mount->path, length) == 0 &&
            (path[length] == '\0' || path[length] == '/')) {
            *relative = path[length] ? path + length : "/";
            return mount;
        }
    }
    return NULL;
}

/* Whether a canonical path is a directory: 1 yes, 0 exists but is not, -1
 * missing or unreadable (errno set). Consults the mounted filesystem for paths
 * below a mount point, which have no node. */
static int directory_status(const char *path) {
    vfs_node_t *node = find_exact(path);
    if (node) {
        return node->is_directory ? 1 : 0;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(path, &relative);
    if (!mount) {
        errno = ENOENT;
        return -1;
    }
    vfs_stat_t stat;
    if (mount->ops->stat(mount->fs, relative, &stat) < 0) {
        return -1;
    }
    return stat.is_directory ? 1 : 0;
}

/* Resolve components while checking intermediate directories, so paths such
 * as /missing/../dev and /dev/uart0/.. cannot bypass lookup errors. */
static int resolve_path(const char *path, char result[VFS_PATH_CAPACITY]) {
    if (!path || !*path) {
        errno = ENOENT;
        return -1;
    }
    strcpy(result, "/");
    size_t length = 1;
    while (*path) {
        while (*path == '/') {
            path++;
        }
        if (!*path) {
            break;
        }
        const char *component = path;
        while (*path && *path != '/') {
            path++;
        }
        size_t size = (size_t)(path - component);
        if (size == 1 && component[0] == '.') {
            continue;
        }
        if (size == 2 && component[0] == '.' && component[1] == '.') {
            while (length > 1 && result[length - 1] != '/') {
                length--;
            }
            if (length > 1) {
                length--;
            }
            result[length] = 0;
            continue;
        }
        size_t slash = length > 1 ? 1 : 0;
        if (length + slash + size >= VFS_PATH_CAPACITY) {
            errno = ENAMETOOLONG;
            return -1;
        }
        if (slash) {
            result[length++] = '/';
        }
        memcpy(result + length, component, size);
        length += size;
        result[length] = 0;
        if (*path) {
            /* A trailing slash may name a directory being created. */
            const char *rest = path;
            while (*rest == '/') {
                rest++;
            }
            int status = directory_status(result);
            if (status < 0 && *rest) {
                return -1;
            }
            if (status == 0) {
                errno = ENOTDIR;
                return -1;
            }
        }
    }
    return 0;
}

int vfs_resolve_path(const char *base, const char *path, char result[VFS_PATH_CAPACITY]) {
    if (!result || !path || !*path) {
        errno = EINVAL;
        return -1;
    }
    if (path[0] == '/') {
        return resolve_path(path, result);
    }
    if (!base || base[0] != '/') {
        errno = EINVAL;
        return -1;
    }
    char directory[VFS_PATH_CAPACITY];
    if (resolve_path(base, directory) < 0) {
        return -1;
    }
    int status = directory_status(directory);
    if (status <= 0) {
        if (status == 0) {
            errno = ENOTDIR;
        }
        return -1;
    }
    size_t base_length = strlen(directory);
    size_t path_length = strlen(path);
    char combined[2 * VFS_PATH_CAPACITY];
    if (path_length >= VFS_PATH_CAPACITY || base_length + 1 + path_length >= sizeof(combined)) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(combined, directory, base_length);
    combined[base_length] = '/';
    memcpy(combined + base_length + 1, path, path_length + 1);
    return resolve_path(combined, result);
}

vfs_node_t *vfs_find_node(const char *name) {
    char path[VFS_PATH_CAPACITY];
    if (resolve_path(name, path) < 0) {
        return NULL;
    }
    vfs_node_t *node = find_exact(path);
    if (!node) {
        errno = ENOENT;
    }
    return node;
}

/* Allocate and register a RAM directory or file for a canonical path that
 * does not exist yet. Returns NULL with errno ENOMEM on allocation failure. */
static vfs_node_t *create_entry(const char *path, bool is_directory) {
    size_t size = strlen(path) + 1;
    vfs_entry_t *entry = calloc(1, sizeof(*entry) + size);
    if (!entry) {
        errno = ENOMEM;
        return NULL;
    }
    memcpy(entry->path, path, size);
    entry->node.name = entry->path;
    entry->node.is_directory = is_directory;
    entry->node.is_regular = !is_directory;
    entry->node.is_owned = true;
    vfs_register_node(&entry->node);
    return &entry->node;
}

/* Unlink an entry created by create_entry() and free it. */
static void remove_entry(vfs_node_t *node) {
    vfs_node_t **link = &vfs_root;
    while (*link && *link != node) {
        link = &(*link)->next;
    }
    if (*link) {
        *link = node->next;
    }
    free((vfs_entry_t *)node); /* node is the entry's first member. */
}

int vfs_mkdir(const char *name) {
    char path[VFS_PATH_CAPACITY];
    if (resolve_path(name, path) < 0) {
        return -1;
    }
    if (find_exact(path)) {
        errno = EEXIST;
        return -1;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(path, &relative);
    if (mount) {
        return mount->ops->mkdir(mount->fs, relative);
    }
    char parent[VFS_PATH_CAPACITY];
    strcpy(parent, path);
    char *slash = strrchr(parent, '/');
    if (slash == parent) {
        slash[1] = 0;
    } else {
        *slash = 0;
    }
    vfs_node_t *node = find_exact(parent);
    if (!node || !node->is_directory) {
        errno = node ? ENOTDIR : ENOENT;
        return -1;
    }
    if (directory_count == CONFIG_HOMECORE_VFS_MAX_DIRECTORIES) {
        errno = ENOSPC;
        return -1;
    }
    if (!create_entry(path, true)) {
        return -1;
    }
    directory_count++;
    return 0;
}

int vfs_rmdir(const char *path) {
    char resolved[VFS_PATH_CAPACITY];
    if (resolve_path(path, resolved) < 0) {
        return -1;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(resolved, &relative);
    if (mount) {
        if (strcmp(relative, "/") == 0) {
            errno = EBUSY; /* Mount points cannot be removed. */
            return -1;
        }
        return mount->ops->rmdir(mount->fs, relative);
    }
    vfs_node_t *node = find_exact(resolved);
    if (!node) {
        errno = ENOENT;
        return -1;
    }
    if (!node->is_directory) {
        errno = ENOTDIR;
        return -1;
    }
    if (node == &root_directory || node == &dev_directory) {
        errno = EBUSY;
        return -1;
    }
    size_t length = strlen(node->name);
    for (vfs_node_t *child = vfs_root; child; child = child->next) {
        if (strncmp(child->name, node->name, length) == 0 && child->name[length] == '/') {
            errno = ENOTEMPTY;
            return -1;
        }
    }
    if (!node->is_owned) {
        errno = EROFS;
        return -1;
    }
    remove_entry(node);
    directory_count--;
    return 0;
}

int vfs_unlink(const char *path) {
    char resolved[VFS_PATH_CAPACITY];
    if (resolve_path(path, resolved) < 0) {
        return -1;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(resolved, &relative);
    if (mount) {
        if (strcmp(relative, "/") == 0) {
            errno = EISDIR;
            return -1;
        }
        return mount->ops->unlink(mount->fs, relative);
    }
    vfs_node_t *node = find_exact(resolved);
    if (!node) {
        errno = ENOENT;
        return -1;
    }
    if (node->is_directory) {
        errno = EISDIR;
        return -1;
    }
    if (!node->is_regular) {
        errno = EPERM;
        return -1;
    }
    for (unsigned i = 0; i < CONFIG_HOMECORE_VFS_MAX_OPEN_FILES; i++) {
        if (vfs_fd_table[i] && vfs_fd_table[i]->node == node) {
            errno = EBUSY;
            return -1;
        }
    }
    free(node->driver_data);
    remove_entry(node);
    ram_file_count--;
    return 0;
}

static vfs_node_t *create_file(const char *name) {
    char path[VFS_PATH_CAPACITY];
    if (resolve_path(name, path) < 0) {
        return NULL;
    }
    size_t length = strlen(name);
    if (name[length - 1] == '/') {
        errno = EISDIR;
        return NULL;
    }
    char parent[VFS_PATH_CAPACITY];
    strcpy(parent, path);
    char *slash = strrchr(parent, '/');
    if (slash == parent) {
        slash[1] = 0;
    } else {
        *slash = 0;
    }
    vfs_node_t *directory = find_exact(parent);
    if (!directory || !directory->is_directory) {
        errno = directory ? ENOTDIR : ENOENT;
        return NULL;
    }
    if (ram_file_count == CONFIG_HOMECORE_VFS_MAX_RAM_FILES) {
        errno = ENOSPC;
        return NULL;
    }
    vfs_node_t *node = create_entry(path, false);
    if (node) {
        ram_file_count++;
    }
    return node;
}

vfs_node_t *vfs_fd_node(int fd) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES || !vfs_fd_table[fd]) {
        errno = EBADF;
        return NULL;
    }
    if (vfs_fd_table[fd]->mount) {
        errno = ENOTSUP;
        return NULL;
    }
    return vfs_fd_table[fd]->node;
}

static void stat_node(const vfs_node_t *node, vfs_stat_t *stat) {
    *stat = (vfs_stat_t){
        .is_directory = node->is_directory,
        .is_regular = node->is_regular,
        .size = node->is_regular ? node->size : 0U,
    };
}

int vfs_stat(const char *path, vfs_stat_t *stat) {
    char resolved[VFS_PATH_CAPACITY];
    if (resolve_path(path, resolved) < 0) {
        return -1;
    }
    vfs_node_t *node = find_exact(resolved);
    if (node) {
        stat_node(node, stat);
        return 0;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(resolved, &relative);
    if (!mount) {
        errno = ENOENT;
        return -1;
    }
    return mount->ops->stat(mount->fs, relative, stat);
}

int vfs_mount(const char *path, const vfs_fs_ops_t *ops, void *fs) {
    if (!path || !ops) {
        errno = EINVAL;
        return -1;
    }
    char resolved[VFS_PATH_CAPACITY];
    if (resolve_path(path, resolved) < 0) {
        return -1;
    }
    const char *relative;
    if (find_mount(resolved, &relative)) {
        errno = EBUSY; /* Mounts do not nest. */
        return -1;
    }
    if (find_exact(resolved)) {
        errno = EEXIST;
        return -1;
    }
    char parent[VFS_PATH_CAPACITY];
    strcpy(parent, resolved);
    char *slash = strrchr(parent, '/');
    if (slash == parent) {
        slash[1] = 0;
    } else {
        *slash = 0;
    }
    vfs_node_t *directory = find_exact(parent);
    if (!directory || !directory->is_directory) {
        errno = directory ? ENOTDIR : ENOENT;
        return -1;
    }
    unsigned slot = 0;
    while (slot < CONFIG_HOMECORE_VFS_MAX_MOUNTS && mounts[slot]) {
        slot++;
    }
    if (slot == CONFIG_HOMECORE_VFS_MAX_MOUNTS) {
        errno = ENOSPC;
        return -1;
    }
    size_t size = strlen(resolved) + 1;
    vfs_mount_t *mount = calloc(1, sizeof(*mount) + size);
    if (!mount) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(mount->path, resolved, size);
    mount->ops = ops;
    mount->fs = fs;
    mount->node.name = mount->path;
    mount->node.is_directory = true;
    vfs_register_node(&mount->node);
    mounts[slot] = mount;
    return 0;
}

int vfs_list(const char *path, vfs_directory_visitor_t visitor, void *context) {
    if (!visitor) {
        errno = EINVAL;
        return -1;
    }
    char resolved[VFS_PATH_CAPACITY];
    if (resolve_path(path, resolved) < 0) {
        return -1;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(resolved, &relative);
    if (mount) {
        return mount->ops->list(mount->fs, relative, visitor, context);
    }
    vfs_node_t *directory = find_exact(resolved);
    if (!directory) {
        errno = ENOENT;
        return -1;
    }
    if (!directory->is_directory) {
        errno = ENOTDIR;
        return -1;
    }
    size_t prefix = strlen(directory->name);
    for (vfs_node_t *node = vfs_root; node; node = node->next) {
        const char *name;
        if (prefix == 1) {
            name = node->name + 1;
        } else {
            if (strncmp(node->name, directory->name, prefix) != 0 || node->name[prefix] != '/') {
                continue;
            }
            name = node->name + prefix + 1;
        }
        if (!*name || strchr(name, '/')) {
            continue;
        }
        int result = visitor(name, node->is_directory, context);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
/* Returns open descriptor fd, or NULL with *status set to -1 for an
 * out-of-range descriptor or -2 for a closed one; errno is not changed. */
static vfs_file_descriptor_t *descriptor(int fd, int *status) {
    if (fd < 0 || fd >= CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        *status = -1;
        return NULL;
    }
    if (!vfs_fd_table[fd]) {
        *status = -2;
        return NULL;
    }
    return vfs_fd_table[fd];
}

/* Allocate a descriptor, with snapshot storage when node has a snapshot
 * operation. Returns NULL with errno ENOMEM on allocation failure. */
static vfs_file_descriptor_t *allocate_descriptor(vfs_node_t *node, int flags) {
    size_t snapshot = (node && node->ops.snapshot) ? VFS_SNAPSHOT_CAPACITY : 0U;
    vfs_file_descriptor_t *file = calloc(1, sizeof(*file) + snapshot);
    if (!file) {
        errno = ENOMEM;
        return NULL;
    }
    file->node = node;
    file->flags = flags;
    return file;
}

/* Open a file below a mount point into free descriptor slot fd. The
 * filesystem handles O_CREAT, O_EXCL, O_TRUNC, and O_APPEND. */
static int open_mounted(int fd, vfs_mount_t *mount, const char *relative, int flags) {
    vfs_file_descriptor_t *file = allocate_descriptor(NULL, flags);
    if (!file) {
        return -1;
    }
    if (mount->ops->open(mount->fs, relative, flags, &file->handle) < 0) {
        int error = errno;
        free(file);
        errno = error;
        return -1;
    }
    file->mount = mount;
    vfs_fd_table[fd] = file;
    return fd;
}

int vfs_open(const char *name, int flags) {
    int access = flags & O_ACCMODE;
    if (access != O_RDONLY && access != O_WRONLY && access != O_RDWR) {
        errno = EINVAL;
        return -1;
    }
    if ((flags & O_TRUNC) && access == O_RDONLY) {
        errno = EACCES;
        return -1;
    }
    int fd = 0;
    while (fd < CONFIG_HOMECORE_VFS_MAX_OPEN_FILES && vfs_fd_table[fd]) {
        fd++;
    }
    if (fd == CONFIG_HOMECORE_VFS_MAX_OPEN_FILES) {
        errno = EMFILE;
        return -1;
    }
    char path[VFS_PATH_CAPACITY];
    if (resolve_path(name, path) < 0) {
        return -1;
    }
    const char *relative;
    vfs_mount_t *mount = find_mount(path, &relative);
    if (mount && strcmp(relative, "/") != 0) {
        return open_mounted(fd, mount, relative, flags);
    }
    vfs_node_t *node = vfs_find_node(name);
    if (node && (flags & O_CREAT) && (flags & O_EXCL)) {
        errno = EEXIST;
        return -1;
    }

    /* Allocate the descriptor before creating or truncating a file, so an
     * allocation failure leaves the namespace and file contents unchanged. */
    vfs_file_descriptor_t *file;
    if (!node && errno == ENOENT && (flags & O_CREAT)) {
        file = allocate_descriptor(NULL, flags);
        if (!file) {
            return -1;
        }
        node = create_file(name);
        if (!node) {
            int error = errno;
            free(file);
            errno = error;
            return -1;
        }
        file->node = node;
    } else {
        if (!node) {
            return -1;
        }
        if (node->is_directory) {
            errno = EISDIR;
            return -1;
        }
        if (node->ops.snapshot &&
            (access != O_RDONLY || (flags & (O_TRUNC | O_APPEND | O_CREAT)))) {
            errno = EACCES;
            return -1;
        }
        file = allocate_descriptor(node, flags);
        if (!file) {
            return -1;
        }
        if (node->is_regular && (flags & O_TRUNC)) {
            free(node->driver_data);
            node->driver_data = NULL;
            node->size = 0;
        }
    }
    vfs_fd_table[fd] = file;

    if (node->ops.open) {
        int result = node->ops.open(node, flags);
        if (result < 0) {
            vfs_fd_table[fd] = NULL;
            free(file);
            return result; // Return the error code from the open operation
        }
    }

    if (node->ops.snapshot) {
        int length = node->ops.snapshot(node, file->snapshot, VFS_SNAPSHOT_CAPACITY);
        if (length < 0 || length > VFS_SNAPSHOT_CAPACITY) {
            vfs_close(fd);
            errno = EIO;
            return -1;
        }
        file->snapshot_length = (unsigned)length;
        file->position = 0;
    }
    return fd;
}
/*---------------------------------------------------------------------------*/
int vfs_close(int fd) {
    int status;
    vfs_file_descriptor_t *file = descriptor(fd, &status);
    if (!file) {
        return status;
    }

    int result = 0;
    if (file->mount) {
        result = file->mount->ops->close(file->mount->fs, file->handle);
    } else if (file->node->ops.close) {
        result = file->node->ops.close(file->node);
    }

    vfs_fd_table[fd] = NULL;
    free(file);
    return result;
}
/*---------------------------------------------------------------------------*/
int vfs_read(int fd, void *buf, unsigned len) {
    int status;
    vfs_file_descriptor_t *file = descriptor(fd, &status);
    if (!file) {
        return status;
    }

    if (file->mount) {
        if ((file->flags & O_ACCMODE) == O_WRONLY) {
            errno = EBADF;
            return -1;
        }
        if (!len) {
            return 0;
        }
        if (!buf) {
            errno = EFAULT;
            return -1;
        }
        return file->mount->ops->read(file->mount->fs, file->handle, buf, len);
    }
    if (file->node->is_regular) {
        if ((file->flags & O_ACCMODE) == O_WRONLY) {
            errno = EBADF;
            return -1;
        }
        if (!len) {
            return 0;
        }
        if (!buf) {
            errno = EFAULT;
            return -1;
        }
        if (file->position >= file->node->size) {
            return 0;
        }
        unsigned available = file->node->size - file->position;
        if (len > available) {
            len = available;
        }
        memcpy(buf, (char *)file->node->driver_data + file->position, len);
        file->position += len;
        return (int)len;
    }
    if (file->node->ops.snapshot) {
        if (len == 0) {
            return 0;
        }
        if (!buf) {
            errno = EFAULT;
            return -1;
        }
        unsigned available = file->snapshot_length - file->position;
        if (len > available) {
            len = available;
        }
        memcpy(buf, file->snapshot + file->position, len);
        file->position += len;
        return (int)len;
    }

    if (file->node->ops.read) {
        return file->node->ops.read(file->node, buf, len);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_write(int fd, const void *buf, unsigned len) {
    int status;
    vfs_file_descriptor_t *file = descriptor(fd, &status);
    if (!file) {
        return status;
    }

    if (file->mount) {
        if ((file->flags & O_ACCMODE) == O_RDONLY) {
            errno = EBADF;
            return -1;
        }
        if (!len) {
            return 0;
        }
        if (!buf) {
            errno = EFAULT;
            return -1;
        }
        if (len > INT_MAX) {
            errno = EOVERFLOW;
            return -1;
        }
        return file->mount->ops->write(file->mount->fs, file->handle, buf, len);
    }
    if (file->node->is_regular) {
        vfs_node_t *node = file->node;
        if ((file->flags & O_ACCMODE) == O_RDONLY) {
            errno = EBADF;
            return -1;
        }
        if (!len) {
            return 0;
        }
        if (!buf) {
            errno = EFAULT;
            return -1;
        }
        unsigned position = (file->flags & O_APPEND) ? node->size : file->position;
        if (len > CONFIG_HOMECORE_VFS_MAX_FILE_SIZE - position) {
            errno = EFBIG;
            return -1;
        }
        unsigned end = position + len;
        if (end > node->size) {
            void *data = realloc(node->driver_data, end);
            if (!data) {
                errno = ENOMEM;
                return -1;
            }
            node->driver_data = data;
            if (position > node->size) {
                memset((char *)data + node->size, 0, position - node->size);
            }
            node->size = end;
        }
        memcpy((char *)node->driver_data + position, buf, len);
        file->position = end;
        return (int)len;
    }
    if (file->node->ops.snapshot) {
        errno = EBADF;
        return -1;
    }

    if (file->node->ops.write) {
        return file->node->ops.write(file->node, buf, len);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_ioctl(int fd, unsigned request, void *arg) {
    int status;
    vfs_file_descriptor_t *file = descriptor(fd, &status);
    if (!file) {
        return status;
    }

    if (file->mount) {
        errno = ENOTTY;
        return -1;
    }
    if (file->node->ops.ioctl) {
        return file->node->ops.ioctl(file->node, request, arg);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_lseek(int fd, int offset, int whence) {
    int status;
    vfs_file_descriptor_t *file = descriptor(fd, &status);
    if (!file) {
        return status;
    }

    if (file->mount) {
        return file->mount->ops->lseek(file->mount->fs, file->handle, offset, whence);
    }
    if (file->node->is_regular) {
        int64_t position = offset;
        if (whence == SEEK_CUR) {
            position += file->position;
        } else if (whence == SEEK_END) {
            position += file->node->size;
        } else if (whence != SEEK_SET) {
            position = -1;
        }
        if (position < 0 || position > CONFIG_HOMECORE_VFS_MAX_FILE_SIZE) {
            errno = EINVAL;
            return -1;
        }
        file->position = (unsigned)position;
        return (int)position;
    }
    if (file->node->ops.snapshot) {
        int64_t position = offset;
        if (whence == SEEK_CUR) {
            position += file->position;
        } else if (whence == SEEK_END) {
            position += file->snapshot_length;
        } else if (whence != SEEK_SET) {
            position = -1;
        }
        if (position < 0 || position > file->snapshot_length) {
            errno = EINVAL;
            return -1;
        }
        file->position = (unsigned)position;
        return (int)position;
    }

    if (file->node->ops.lseek) {
        return file->node->ops.lseek(file->node, offset, whence);
    }

    return -1;
}
/*---------------------------------------------------------------------------*/
int vfs_fstat(int fd, vfs_stat_t *stat) {
    int status;
    vfs_file_descriptor_t *file = descriptor(fd, &status);
    if (!file) {
        errno = EBADF;
        return -1;
    }
    if (file->mount) {
        return file->mount->ops->fstat(file->mount->fs, file->handle, stat);
    }
    stat_node(file->node, stat);
    return 0;
}
/*---------------------------------------------------------------------------*/
