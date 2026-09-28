/* SPDX-License-Identifier: MIT */
#include "homecore/kernel/kernel.h"
#include "homecore/arch/arch.h"
#include "homecore/board/board.h"
#include "homecore/vfs/vfs.h"
#include <string.h>

static volatile uint64_t uptime_ms;

void k_tick(void) {
    uptime_ms++;
}

uint64_t k_uptime_ms(void) {
    arch_irq_key_t key = arch_irq_lock();
    uint64_t value = uptime_ms;
    arch_irq_unlock(key);
    return value;
}

static int uptime_snapshot(vfs_node_t *node, char *buf, unsigned capacity) {
    (void)node;
    /* Avoid newlib-nano's optional 64-bit printf support. */
    char digits[21];
    unsigned start = sizeof(digits);
    digits[--start] = '\n';
    uint64_t value = k_uptime_ms();
    do {
        digits[--start] = '0' + value % 10;
        value /= 10;
    } while (value);
    unsigned length = sizeof(digits) - start;
    if (capacity < length) {
        return -1;
    }
    memcpy(buf, digits + start, length);
    return (int)length;
}

static vfs_node_t uptime_node = {
    .name = "/dev/uptime",
    .ops = {.snapshot = uptime_snapshot},
};

void k_uptime_init(void) {
    if (arch_cpu_timer_init(board_cpu_clock_hz(), 1000) != 0) {
        board_panic("Cannot initialize uptime timer");
        return;
    }
    vfs_register_node(&uptime_node);
}
