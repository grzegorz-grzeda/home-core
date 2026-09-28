/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief Sessions: a user identity, working directory, and last command status.
 */
#ifndef HOMECORE_SESSION_H
#define HOMECORE_SESSION_H
#include "homecore/user/user.h"
#include "homecore/vfs/vfs.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup session Sessions
 * @ingroup kernel_services
 * @brief Per-session execution state shared by the shell and libc path hooks.
 *
 * One session is current at a time. It is stored in a single global pointer,
 * which a future scheduler must make task-local. The default session runs as
 * `root` with working directory `/`. These functions are not interrupt-safe.
 * @{
 */

/** @brief Execution state of one shell session. */
typedef struct {
    /** Identity. Not owned; must outlive the session. */
    const user_t *user;
    /** Canonical absolute working directory. */
    char cwd[VFS_PATH_CAPACITY];
    /** Status of the last command run in this session. */
    int last_status;
} session_t;

/**
 * @brief Initialize a session for a user in the user's home directory.
 *
 * @param[out] session Session to initialize. Unchanged on failure.
 * @param      user    Identity with a name and an absolute home directory.
 *                     Must outlive the session.
 *
 * @retval 0  The session is initialized with status 0.
 * @retval -1 `errno` is `EINVAL` for a missing argument, name, or home, or a
 *            relative home; otherwise it is set by session_chdir().
 */
int session_init(session_t *session, const user_t *user);
/**
 * @brief Change a session's working directory.
 *
 * @param session Session to update.
 * @param path    Absolute path, or a path relative to the current directory.
 *
 * @retval 0  `cwd` now names the directory.
 * @retval -1 `cwd` is unchanged; `errno` is `EINVAL`, `ENOENT`, `ENOTDIR`, or
 *            `ENAMETOOLONG`.
 */
int session_chdir(session_t *session, const char *path);
/**
 * @brief Get the current session.
 *
 * libc path operations resolve relative paths against this session.
 *
 * @return The current session; never `NULL`.
 */
session_t *session_current(void);
/**
 * @brief Make a session current.
 *
 * The pointer is retained, so the session must stay valid while it is current.
 *
 * @param session Session to activate, or `NULL` for the default root session.
 *
 * @return The previously current session, to restore later.
 */
session_t *session_set_current(session_t *session);

/** @} */

#ifdef __cplusplus
}
#endif
#endif
