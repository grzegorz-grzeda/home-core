/* SPDX-License-Identifier: MIT */
#include "homecore/vfs/vfs.h"
#include "homecore/autoconf.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static char entries[512];
static int collect(const char *name, bool directory, void *context) {
    (void)context;
    strcat(entries, name);
    strcat(entries, directory ? "/\n" : "\n");
    return 0;
}
static int stop(const char *name, bool directory, void *context) {
    (void)name;
    (void)directory;
    (void)context;
    return 7;
}
static vfs_node_t device = {.name = "/dev/test"};
int main(void) {
    vfs_register_node(&device);
    vfs_register_node(&device); /* Must not create a cycle. */
    assert(vfs_list("/", collect, NULL) == 0);
    assert(strcmp(entries, "dev/\n") == 0);
    entries[0] = 0;
    assert(vfs_list("/dev", collect, NULL) == 0);
    assert(strcmp(entries, "test\n") == 0);
    assert(vfs_mkdir("tmp") == 0);
    assert(vfs_mkdir("/tmp/child/") == 0);
    assert(vfs_find_node("//tmp/./child/../child")->is_directory);
    assert(vfs_find_node("/tmp/child/../../dev/test") == &device);
    assert(vfs_mkdir("/tmp") == -1 && errno == EEXIST);
    assert(vfs_mkdir("/missing/child") == -1 && errno == ENOENT);
    assert(vfs_mkdir("/dev/test/child") == -1 && errno == ENOTDIR);
    assert(!vfs_find_node("/missing/../dev") && errno == ENOENT);
    assert(!vfs_find_node("/dev/test/..") && errno == ENOTDIR);
    assert(!vfs_find_node("/dev/test/") && errno == ENOTDIR);
    assert(!vfs_find_node("") && errno == ENOENT);
    assert(vfs_open("/tmp", O_RDONLY) == -1 && errno == EISDIR);
    assert(vfs_list("/dev/test", collect, NULL) == -1 && errno == ENOTDIR);
    assert(vfs_list("/", stop, NULL) == 7);
    entries[0] = 0;
    assert(vfs_list("/tmp/child", collect, NULL) == 0 && !entries[0]);
    entries[0] = 0;
    assert(vfs_list("/tmp", collect, NULL) == 0 && strcmp(entries, "child/\n") == 0);
    char oversized[VFS_PATH_CAPACITY + 1];
    memset(oversized, 'a', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = 0;
    assert(vfs_mkdir(oversized) == -1 && errno == ENAMETOOLONG);
    for (int i = 2; i < CONFIG_HOMECORE_VFS_MAX_DIRECTORIES; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/dir%d", i);
        assert(vfs_mkdir(path) == 0);
    }
    assert(vfs_mkdir("/full") == -1 && errno == ENOSPC);
    puts("PASS: listings, normalization, hierarchy errors, duplicates, directory capacity");
}
