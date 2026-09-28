/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief User identity record.
 */
#ifndef HOMECORE_USER_H
#define HOMECORE_USER_H
#include <stdint.h>

/**
 * @defgroup user Users
 * @ingroup kernel_services
 * @brief User identity metadata.
 *
 * Identity only: there is no authentication, ownership, or permission check.
 * @{
 */

/** @brief User identity. The strings are not owned and must outlive users of the record. */
typedef struct {
    /** Numeric user ID; 0 is `root`. */
    uint32_t uid;
    /** Primary group ID. */
    uint32_t gid;
    /** Login name. */
    const char *name;
    /** Absolute home directory, used by session_init() and `cd` without arguments. */
    const char *home;
} user_t;

/** @} */
#endif
