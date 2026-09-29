/* SPDX-License-Identifier: MIT */
#include "homecore/session/session.h"
#include <errno.h>
#include <string.h>

static const user_t root_user = {.uid = 0, .gid = 0, .name = "root", .home = "/"};
static session_t root_session = {.user = &root_user, .cwd = "/"};
static session_t *current_session = &root_session;

int session_chdir(session_t *session, const char *path) {
    if (!session) {
        errno = EINVAL;
        return -1;
    }
    char resolved[VFS_PATH_CAPACITY];
    if (vfs_resolve_path(session->cwd, path, resolved) < 0) {
        return -1;
    }
    vfs_stat_t stat;
    if (vfs_stat(resolved, &stat) < 0) {
        return -1;
    }
    if (!stat.is_directory) {
        errno = ENOTDIR;
        return -1;
    }
    strcpy(session->cwd, resolved);
    return 0;
}

int session_init(session_t *session, const user_t *user) {
    if (!session || !user || !user->name || !user->home || user->home[0] != '/') {
        errno = EINVAL;
        return -1;
    }
    session_t initialized = {.user = user, .cwd = "/", .last_status = 0};
    if (session_chdir(&initialized, user->home) < 0) {
        return -1;
    }
    *session = initialized;
    return 0;
}

session_t *session_current(void) {
    return current_session;
}

session_t *session_set_current(session_t *session) {
    session_t *previous = current_session;
    current_session = session ? session : &root_session;
    return previous;
}
