/* SPDX-License-Identifier: MIT */
#include "builtin_system.h"
#include "homecore/kernel/kernel.h"
#include "homecore/arch/arch.h"
#include <stdio.h>

static int no_arguments(int argc, char **argv) {
    if (argc == 1) {
        return 1;
    }
    printf("Usage: %s\n", argv[0]);
    return 0;
}

int shell_builtin_mem(shell_context_t *context, int argc, char **argv) {
    (void)context;
    if (!no_arguments(argc, argv)) {
        return 1;
    }
    k_heap_stats_t stats;
    k_heap_stats(&stats);
    printf("Heap total: %lu bytes\nAllocated: %lu bytes\nReusable: %lu bytes\n"
           "Unclaimed: %lu bytes\nAvailable: %lu bytes\n",
           (unsigned long)stats.total,
           (unsigned long)stats.allocated,
           (unsigned long)stats.reusable,
           (unsigned long)stats.unclaimed,
           (unsigned long)(stats.reusable + stats.unclaimed));
    k_stack_stats_t stack;
    k_stack_stats(&stack);
    printf("Stack: used %lu of %lu bytes\n", (unsigned long)stack.used, (unsigned long)stack.size);
    return 0;
}

int shell_builtin_uptime(shell_context_t *context, int argc, char **argv) {
    (void)context;
    if (!no_arguments(argc, argv)) {
        return 1;
    }
    uint64_t ms = k_uptime_ms();
    uint64_t seconds = ms / 1000;
    /* A uint64 millisecond counter's day count fits in 12 decimal digits. */
    uint64_t days = seconds / 86400;
    char digits[13];
    unsigned end = sizeof(digits);
    digits[--end] = 0;
    do {
        digits[--end] = '0' + days % 10;
        days /= 10;
    } while (days);
    printf("up %s days, %02lu:%02lu:%02lu.%03lu\n",
           digits + end,
           (unsigned long)(seconds / 3600 % 24),
           (unsigned long)(seconds / 60 % 60),
           (unsigned long)(seconds % 60),
           (unsigned long)(ms % 1000));
    return 0;
}

int shell_builtin_clear(shell_context_t *context, int argc, char **argv) {
    (void)context;
    if (!no_arguments(argc, argv)) {
        return 1;
    }
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
    return 0;
}

int shell_builtin_reboot(shell_context_t *context, int argc, char **argv) {
    (void)context;
    if (!no_arguments(argc, argv)) {
        return 1;
    }
    puts("Rebooting...");
    fflush(stdout);
    arch_cpu_reset();
    return 0;
}
