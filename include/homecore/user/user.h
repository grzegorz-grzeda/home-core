/* SPDX-License-Identifier: MIT */
#ifndef HOMECORE_USER_H
#define HOMECORE_USER_H
#include <stdint.h>
typedef struct {
    uint32_t uid;
    uint32_t gid;
    const char *name;
    const char *home;
} user_t;
#endif
