/* SPDX-License-Identifier: MIT */
#ifndef HOMECORE_SESSION_H
#define HOMECORE_SESSION_H
#include "homecore/user/user.h"
#include "homecore/vfs/vfs.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const user_t *user;
    char cwd[VFS_PATH_CAPACITY];
    int last_status;
} session_t;

/* The user object must outlive its session. Initializes CWD from user->home. */
int session_init(session_t *session, const user_t *user);
int session_chdir(session_t *session, const char *path);
/* Single-threaded execution context, including libc path resolution.
 * A future scheduler must make this task-local. Default: root, home "/". */
session_t *session_current(void);
session_t *session_set_current(session_t *session);

#ifdef __cplusplus
}
#endif
#endif
