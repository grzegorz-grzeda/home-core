/* SPDX-License-Identifier: MIT */
#include "homecore/shell/shell.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int shell_builtin_basic(shell_context_t *context, int argc, char **argv) {
    (void)context; (void)argc; (void)argv;
    return 0;
}
static int check_context(shell_context_t *context, int argc, char **argv) {
    assert(session_current() == context);
    assert(argc == 1 && argv[argc] == NULL);
    return 23;
}
static int run(shell_context_t *context, const char *text) {
    char line[256];
    strcpy(line, text);
    return shell_execute_line(context, line);
}
int main(void) {
    assert(vfs_mkdir("/alice") == 0);
    assert(vfs_mkdir("/bob") == 0);
    user_t alice = {.uid = 100, .gid = 10, .name = "alice", .home = "/alice"};
    user_t bob = {.uid = 101, .gid = 10, .name = "bob", .home = "/bob"};
    shell_context_t a, b;
    assert(session_init(&a, &alice) == 0);
    assert(session_init(&b, &bob) == 0);
    assert(strcmp(a.cwd, "/alice") == 0 && strcmp(b.cwd, "/bob") == 0);
    session_t *original = session_current();
    assert(original->user->uid == 0 && strcmp(original->cwd, "/") == 0);
    shell_init();
    shell_register_command("check-context", "test", check_context);
    assert(run(&a, "check-context") == 23 && a.last_status == 23);
    assert(session_current() == original);
    assert(run(&a, "mkdir child") == 0);
    assert(vfs_find_node("/alice/child") && !vfs_find_node("/bob/child"));
    assert(run(&a, "cd child") == 0 && strcmp(a.cwd, "/alice/child") == 0);
    assert(strcmp(b.cwd, "/bob") == 0);
    assert(run(&a, "cd /missing") == 1 && a.last_status == 1);
    assert(strcmp(a.cwd, "/alice/child") == 0);
    assert(run(&a, "cd ..") == 0 && strcmp(a.cwd, "/alice") == 0);
    assert(run(&a, "cd /") == 0 && strcmp(a.cwd, "/") == 0);
    assert(run(&a, "cd") == 0 && strcmp(a.cwd, "/alice") == 0);
    assert(run(&a, "pwd") == 0);
    assert(run(&a, "whoami") == 0);
    assert(run(&a, "id") == 0);
    assert(run(&b, "not-a-command") == 127 && b.last_status == 127);
    assert(a.last_status == 0);
    assert(run(&b, "  \t") == 127);
    char result[VFS_PATH_CAPACITY];
    assert(vfs_resolve_path("/alice", "../bob", result) == 0);
    assert(strcmp(result, "/bob") == 0);
    assert(vfs_resolve_path("/missing", "/dev", result) == 0);
    assert(vfs_resolve_path("/missing", "dev", result) < 0);
    assert(vfs_resolve_path("/", "missing/../dev", result) < 0);
    assert(session_set_current(&a) == original);
    assert(session_current() == &a);
    assert(session_set_current(original) == &a);
    user_t bad = {.name = "bad", .home = "/absent"};
    assert(session_init(&a, &bad) < 0 && a.user == &alice);
    assert(session_current() == original);
    puts("PASS: independent sessions, cwd, home, identity, status, execution context");
}
