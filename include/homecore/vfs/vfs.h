/*
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
/**
 * @file
 * @brief Virtual file system: device nodes, RAM files and directories, paths,
 *        and descriptors.
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
/**
 * @defgroup vfs Virtual file system
 * @ingroup subsystems
 * @brief Single namespace of device nodes, RAM directories, and RAM files.
 *
 * Nodes are kept in one list and are named by canonical absolute paths.
 * RAM directories, RAM files, and descriptors are allocated from the heap
 * when created or opened and freed when removed or closed; the Kconfig maxima
 * cap how many can exist. RAM file contents also use the heap. Everything is
 * lost on reset. libc I/O reaches
 * the VFS through the newlib hooks in `src/kernel/syscalls.c`. The VFS is not
 * reentrant and must not be called from interrupt handlers.
 *
 * Paths passed to these functions are resolved from `/`, even when they do not
 * start with a slash. To honor a session's working directory, resolve the path
 * with vfs_resolve_path() first.
 *
 * Return convention: failures return -1 with `errno` set, except where a
 * function documents otherwise. Descriptor functions return -1 for an
 * out-of-range descriptor and -2 for a closed descriptor without setting
 * `errno`. Callers should therefore test for a negative result.
 * @{
 */
/*---------------------------------------------------------------------------*/
/** @name Limits
 * @{
 */
/** @brief Maximum content size of a snapshot device, in bytes. */
#define VFS_SNAPSHOT_CAPACITY 32
/** @brief Path buffer size including the terminator; paths hold at most 127 bytes. */
#define VFS_PATH_CAPACITY 128
/** @} */
/*---------------------------------------------------------------------------*/
/** @name Nodes and drivers
 * @{
 */
/** @brief VFS node; see struct vfs_node. */
typedef struct vfs_node vfs_node_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Device operations.
 *
 * Every operation is optional; leave unused ones `NULL`. A missing stream
 * operation makes the matching descriptor call return -1 without setting
 * `errno`. The VFS implements RAM files itself and ignores these operations
 * for them.
 */
typedef struct {
    /**
     * Called by vfs_open() after a descriptor is reserved. A negative result
     * releases the descriptor and is returned by vfs_open().
     */
    int (*open)(vfs_node_t *node, int flags);
    /** Called by vfs_close(), which returns its result. The descriptor is released anyway. */
    int (*close)(vfs_node_t *node);

    /** Stream read. Return the byte count, 0 at end of file, or -1 with `errno` set. */
    int (*read)(vfs_node_t *node, void *buf, unsigned len);
    /** Stream write. Return the byte count, or -1 with `errno` set. */
    int (*write)(vfs_node_t *node, const void *buf, unsigned len);

    /** Device-specific control request, forwarded unchanged by vfs_ioctl(). */
    int (*ioctl)(vfs_node_t *node, unsigned request, void *arg);

    /** Stream seek, forwarded unchanged by vfs_lseek(). */
    int (*lseek)(vfs_node_t *node, int offset, int whence);

    /**
     * Optional read-only snapshot generated at open. Write at most @p capacity
     * bytes into @p buf and return the length, or -1 if the content does not
     * fit. The VFS then handles partial reads, end of file, and seeking
     * independently for each descriptor, and ignores `read`, `write`, and
     * `lseek`. Snapshot nodes can be opened only with `O_RDONLY` and no
     * creation, truncation, or append flags.
     */
    int (*snapshot)(vfs_node_t *node, char *buf, unsigned capacity);
} vfs_ops_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Named entry in the VFS namespace.
 *
 * Drivers define device nodes statically, setting `name`, `ops`, and
 * optionally `driver_data`, and pass them to vfs_register_node(). The VFS
 * manages the other fields.
 */
typedef struct vfs_node {
    /** Canonical absolute path, such as `/dev/uart0`. Must outlive registration. */
    const char *name;
    /** Device operations. */
    vfs_ops_t ops;
    /** Driver-private state. For RAM files, the VFS-owned heap contents. */
    void *driver_data;
    /** Namespace list link, managed by the VFS. */
    vfs_node_t *next;
    /** The node is a directory. */
    bool is_directory;
    /** The node is a RAM file managed by the VFS. Device drivers leave it `false`. */
    bool is_regular;
    /** The VFS allocated the node and frees it on removal. Drivers leave it `false`. */
    bool is_owned;
    /** RAM file length in bytes. Unused for devices and directories. */
    unsigned size;
} vfs_node_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Obsolete initializer; currently does nothing.
 *
 * VFS state is statically initialized and devices are registered during
 * startup. Do not call it after nodes are registered.
 */
void vfs_init(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Add a node to the namespace.
 *
 * The node is linked by pointer and cannot be unregistered, so it must remain
 * valid for the rest of the program. A `NULL` node, a `NULL` name, or a name
 * that is already registered is silently ignored. The parent directory is not
 * checked. Device nodes belong under `/dev`.
 *
 * @param node Node to register.
 */
void vfs_register_node(vfs_node_t *node);
/*---------------------------------------------------------------------------*/
/**
 * @brief Look up a node by path.
 *
 * @param name Path to resolve from `/`. Intermediate components must be
 *             existing directories.
 *
 * @return The node, or `NULL` with `errno` set to `ENOENT`, `ENOTDIR`, or
 *         `ENAMETOOLONG`.
 */
vfs_node_t *vfs_find_node(const char *name);
/** @} */
/*---------------------------------------------------------------------------*/
/** @name Namespace operations
 * @{
 */
/**
 * @brief Resolve a path against an explicit base directory.
 *
 * Absolute paths ignore @p base. Repeated slashes, `.`, and `..` are
 * normalized. Every intermediate component must be an existing directory, but
 * the final component may not exist. Other VFS operations remain rooted at `/`
 * for relative arguments.
 *
 * @param base   Existing absolute directory. Used only when @p path is
 *               relative.
 * @param path   Path to resolve. Must not be empty.
 * @param[out] result Canonical absolute path. Must hold #VFS_PATH_CAPACITY
 *               bytes.
 *
 * @retval 0  @p result holds the resolved path.
 * @retval -1 `errno` is `EINVAL` (missing argument, empty path, or a relative
 *            path with a missing or relative base), `ENOENT`, `ENOTDIR`, or
 *            `ENAMETOOLONG`.
 */
int vfs_resolve_path(const char *base, const char *path, char result[VFS_PATH_CAPACITY]);
/*---------------------------------------------------------------------------*/
/**
 * @brief Create an empty RAM directory.
 *
 * @param path Directory to create. Its parent must exist.
 *
 * @retval 0  The directory was created.
 * @retval -1 `errno` is `EEXIST`, `ENOENT`, `ENOTDIR`, `ENAMETOOLONG`,
 *            `ENOSPC` when `CONFIG_HOMECORE_VFS_MAX_DIRECTORIES` directories
 *            exist, or `ENOMEM` when the entry cannot be allocated.
 */
int vfs_mkdir(const char *path);
/*---------------------------------------------------------------------------*/
/**
 * @brief Remove an empty directory created by vfs_mkdir().
 *
 * Does not check whether a session uses the directory as its working
 * directory; callers must check that themselves.
 *
 * @param path Directory to remove.
 *
 * @retval 0  The directory was removed and its memory freed.
 * @retval -1 `errno` is `ENOENT`, `ENOTDIR`, `EBUSY` for `/` and `/dev`,
 *            `ENOTEMPTY`, or `EROFS` for a registered directory that was not
 *            created by vfs_mkdir().
 */
int vfs_rmdir(const char *path);
/*---------------------------------------------------------------------------*/
/**
 * @brief Remove a RAM file and free its contents.
 *
 * @param path File to remove.
 *
 * @retval 0  The file was removed and its memory freed.
 * @retval -1 `errno` is `ENOENT`, `EISDIR`, `EPERM` for a device node, or
 *            `EBUSY` while any descriptor is open on the file.
 */
int vfs_unlink(const char *path);
/*---------------------------------------------------------------------------*/
/**
 * @brief Visitor called for each directory entry by vfs_list().
 *
 * @param name         Entry name without its parent path. Valid only for the
 *                     duration of the call.
 * @param is_directory `true` if the entry is a directory.
 * @param context      Value passed to vfs_list().
 *
 * @return 0 to continue, or nonzero to stop and make vfs_list() return this
 *         value.
 */
typedef int (*vfs_directory_visitor_t)(const char *name, bool is_directory, void *context);
/*---------------------------------------------------------------------------*/
/**
 * @brief Visit the direct children of a directory.
 *
 * Entries are visited most recently registered first. The namespace must not
 * be modified during the visit.
 *
 * @param path    Directory to list.
 * @param visitor Function called for each child.
 * @param context Value passed to @p visitor.
 *
 * @return 0 after visiting every child; the visitor's nonzero result if it
 *         stopped early; or -1 with `errno` set to `EINVAL` for a `NULL`
 *         visitor, `ENOENT`, or `ENOTDIR`.
 */
int vfs_list(const char *path, vfs_directory_visitor_t visitor, void *context);
/** @} */
/*---------------------------------------------------------------------------*/
/** @name Descriptor operations
 * @{
 */
/**
 * @brief Open a node and allocate the lowest free descriptor.
 *
 * `O_CREAT` creates a RAM file when the parent directory exists. `O_TRUNC`
 * discards a RAM file's contents. Opening a snapshot device captures its
 * content for this descriptor.
 *
 * @param name  Path to open, resolved from `/`.
 * @param flags One of `O_RDONLY`, `O_WRONLY`, or `O_RDWR`, optionally combined
 *              with `O_CREAT`, `O_EXCL`, `O_TRUNC`, and `O_APPEND` from
 *              `<fcntl.h>`.
 *
 * @return A descriptor from 0 to `CONFIG_HOMECORE_VFS_MAX_OPEN_FILES - 1`;
 *         a negative result from the node's `open` operation; or -1 with
 *         `errno` set to `EINVAL` (invalid access mode), `EACCES` (`O_TRUNC`
 *         with `O_RDONLY`, or write access to a snapshot device), `EMFILE`,
 *         `EEXIST`, `EISDIR`, `ENOENT`, `ENOTDIR`, `ENAMETOOLONG`, `ENOSPC`
 *         (`CONFIG_HOMECORE_VFS_MAX_RAM_FILES` files exist), `ENOMEM` (the
 *         descriptor or new file cannot be allocated; nothing is created or
 *         truncated), or `EIO` (snapshot failed).
 */
int vfs_open(const char *name, int flags);
/*---------------------------------------------------------------------------*/
/**
 * @brief Release a descriptor.
 *
 * @param fd Descriptor returned by vfs_open().
 *
 * @return The node's `close` result, or 0 if it has none; -1 for an
 *         out-of-range descriptor; -2 for a closed descriptor.
 */
int vfs_close(int fd);
/*---------------------------------------------------------------------------*/
/**
 * @brief Read from a descriptor.
 *
 * RAM files and snapshot devices copy from the descriptor's position and
 * advance it. Other nodes forward to their `read` operation, which may block.
 *
 * @param fd  Open descriptor.
 * @param buf Destination of at least @p len bytes. May be `NULL` only if
 *            @p len is 0.
 * @param len Maximum number of bytes to read.
 *
 * @return Bytes read, or 0 at end of file; -1 on failure (`errno` is `EBADF`
 *         for a write-only RAM file, `EFAULT` for a `NULL` buffer, or set by
 *         the driver); -2 for a closed descriptor.
 */
int vfs_read(int fd, void *buf, unsigned len);
/*---------------------------------------------------------------------------*/
/**
 * @brief Write to a descriptor.
 *
 * RAM files are written at the descriptor's position, or at the end with
 * `O_APPEND`. A write past the end grows the file and zero-fills any gap. Other
 * nodes forward to their `write` operation.
 *
 * @param fd  Open descriptor.
 * @param buf Source of at least @p len bytes. May be `NULL` only if @p len
 *            is 0.
 * @param len Number of bytes to write.
 *
 * @return Bytes written; -1 on failure (`errno` is `EBADF` for a read-only
 *         descriptor or snapshot device, `EFAULT`, `EFBIG` beyond
 *         `CONFIG_HOMECORE_VFS_MAX_FILE_SIZE`, `ENOMEM`, or set by the
 *         driver); -2 for a closed descriptor.
 */
int vfs_write(int fd, const void *buf, unsigned len);
/*---------------------------------------------------------------------------*/
/**
 * @brief Forward a control request to a device.
 *
 * @param fd      Open descriptor.
 * @param request Device-specific request code.
 * @param arg     Device-specific argument.
 *
 * @return The node's `ioctl` result; -1 if it has none; -2 for a closed
 *         descriptor.
 */
int vfs_ioctl(int fd, unsigned request, void *arg);
/*---------------------------------------------------------------------------*/
/**
 * @brief Move a descriptor's position.
 *
 * RAM files accept positions from 0 to `CONFIG_HOMECORE_VFS_MAX_FILE_SIZE`.
 * Snapshot devices accept positions within the captured content. Other nodes
 * forward to their `lseek` operation.
 *
 * @param fd     Open descriptor.
 * @param offset Offset relative to @p whence.
 * @param whence `SEEK_SET`, `SEEK_CUR`, or `SEEK_END`.
 *
 * @return New position; -1 on failure (`errno` is `EINVAL` for an invalid
 *         @p whence or out-of-range position, or set by the driver); -2 for a
 *         closed descriptor.
 */
int vfs_lseek(int fd, int offset, int whence);
/*---------------------------------------------------------------------------*/
/**
 * @brief Get the node behind an open descriptor.
 *
 * @param fd Descriptor to inspect.
 *
 * Descriptors on mounted filesystems have no node; use vfs_fstat() for them.
 *
 * @return The node, or `NULL` with `errno` set to `EBADF` for a closed or
 *         out-of-range descriptor, or `ENOTSUP` for a file on a mounted
 *         filesystem.
 */
vfs_node_t *vfs_fd_node(int fd);
/*---------------------------------------------------------------------------*/
/** @brief File type and size reported by vfs_stat() and vfs_fstat(). */
typedef struct {
    /** The path is a directory, including a mount point. */
    bool is_directory;
    /** The path is a regular file: a RAM file or a file on a mounted filesystem. */
    bool is_regular;
    /** File length in bytes; 0 for directories and devices. */
    unsigned size;
} vfs_stat_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Report the type and size of a path.
 *
 * Works for nodes and for paths inside mounted filesystems, which have no
 * node. A path that is neither a directory nor a regular file is a device.
 *
 * @param path      Path to inspect, resolved from `/`.
 * @param[out] stat Result. Must not be `NULL`.
 *
 * @retval 0  @p stat is filled in.
 * @retval -1 `errno` is `ENOENT`, `ENOTDIR`, `ENAMETOOLONG`, or set by the
 *            mounted filesystem.
 */
int vfs_stat(const char *path, vfs_stat_t *stat);
/*---------------------------------------------------------------------------*/
/**
 * @brief Report the type and size of an open descriptor.
 *
 * @param fd        Open descriptor.
 * @param[out] stat Result. Must not be `NULL`.
 *
 * @return 0 on success; -1 with `errno` set to `EBADF` for a closed or
 *         out-of-range descriptor, or set by the mounted filesystem.
 */
int vfs_fstat(int fd, vfs_stat_t *stat);
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * @name Mounted filesystems
 *
 * A mounted filesystem owns everything below its mount point. Paths passed to
 * its operations are relative to the mount point and always start with `/`
 * (the mount point itself is `/`). Operations return 0 or a byte count on
 * success and -1 with `errno` set on failure, like the corresponding `vfs_*`
 * functions. The VFS resolves `.` and `..`, rejects reads and writes the
 * access mode forbids, zero-length transfers, and `NULL` buffers, and allocates
 * the descriptor; the filesystem handles `O_CREAT`, `O_EXCL`, `O_TRUNC`, and
 * `O_APPEND`. Descriptors on mounted files do not support vfs_ioctl()
 * (`ENOTTY`).
 * @{
 */
/** @brief Operations a mounted filesystem provides; every one is required. */
typedef struct {
    /** Open @p path with `open()` flags; store the file handle in @p file. */
    int (*open)(void *fs, const char *path, int flags, void **file);
    /** Close and free a file handle. */
    int (*close)(void *fs, void *file);
    /** Read up to @p len bytes; 0 at end of file. */
    int (*read)(void *fs, void *file, void *buf, unsigned len);
    /** Write @p len bytes; a short count means the filesystem is full. */
    int (*write)(void *fs, void *file, const void *buf, unsigned len);
    /** Move the file position; return the new position. */
    int (*lseek)(void *fs, void *file, int offset, int whence);
    /** Report an open file's type and size. */
    int (*fstat)(void *fs, void *file, vfs_stat_t *stat);
    /** Report a path's type and size; `ENOENT` if it does not exist. */
    int (*stat)(void *fs, const char *path, vfs_stat_t *stat);
    /** Create a directory whose parent exists. */
    int (*mkdir)(void *fs, const char *path);
    /** Remove an empty directory. */
    int (*rmdir)(void *fs, const char *path);
    /** Remove a closed regular file. */
    int (*unlink)(void *fs, const char *path);
    /** Call @p visitor for each entry of a directory, as vfs_list() does. */
    int (*list)(void *fs, const char *path, vfs_directory_visitor_t visitor, void *context);
} vfs_fs_ops_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Attach a filesystem at a new mount point.
 *
 * Creates @p path as a directory node owned by the filesystem. Mount points
 * cannot be removed or nested, and there is no unmount yet.
 *
 * @param path Absolute path of the mount point. Its parent must exist and the
 *             path must not. The string is copied.
 * @param ops  Filesystem operations. Must remain valid for the rest of the
 *             program.
 * @param fs   Filesystem instance passed to every operation.
 *
 * @retval 0  The filesystem is mounted.
 * @retval -1 `errno` is `EINVAL` (missing argument), `EEXIST`, `ENOENT`,
 *            `ENOTDIR`, `ENAMETOOLONG`, `EBUSY` (inside another mount), or
 *            `ENOSPC` when `CONFIG_HOMECORE_VFS_MAX_MOUNTS` mounts exist.
 */
int vfs_mount(const char *path, const vfs_fs_ops_t *ops, void *fs);
/** @} */
/*---------------------------------------------------------------------------*/
/** @} */
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_VFS_H
/*---------------------------------------------------------------------------*/
