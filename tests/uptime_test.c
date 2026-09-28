/* SPDX-License-Identifier: MIT */
/* Host test: cc -Iinclude -Ibuild/lm3s6965evb/include tests/uptime_test.c
 * src/subsystems/vfs/vfs.c -o /tmp/uptime-test && /tmp/uptime-test */
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include "../src/kernel/uptime.c"

static int locked;
arch_irq_key_t arch_irq_lock(void) {
    arch_irq_key_t old = locked;
    locked = 1;
    return old;
}
void arch_irq_unlock(arch_irq_key_t key) {
    locked = key;
}
uint32_t board_cpu_clock_hz(void) {
    return 12500000;
}
int arch_cpu_timer_init(uint32_t cpu_hz, uint32_t tick_hz) {
    assert(cpu_hz == 12500000 && tick_hz == 1000);
    return 0;
}
void board_panic(const char *message) {
    (void)message;
    assert(0);
}

int main(void) {
    k_uptime_init();
    for (int i = 0; i < 1234; i++) {
        k_tick();
    }
    assert(k_uptime_ms() == 1234 && locked == 0);
    locked = 1;
    assert(k_uptime_ms() == 1234 && locked == 1);
    locked = 0;
    assert(vfs_open("/dev/uptime", O_WRONLY) < 0);
    assert(vfs_open("/dev/uptime", O_RDWR) < 0);
    int first = vfs_open("/dev/uptime", O_RDONLY);
    assert(first >= 0);
    k_tick();
    int second = vfs_open("/dev/uptime", O_RDONLY);
    assert(second >= 0 && second != first);
    char buf[32] = {0};
    assert(vfs_read(first, NULL, 0) == 0);
    assert(vfs_read(first, buf, 2) == 2);
    assert(vfs_read(first, buf + 2, 30) == 3);
    assert(strcmp(buf, "1234\n") == 0);
    assert(vfs_read(first, buf, 32) == 0);
    assert(vfs_write(first, "x", 1) < 0);
    assert(vfs_read(second, buf, 32) == 5 && memcmp(buf, "1235\n", 5) == 0);
    assert(vfs_lseek(first, 0, SEEK_SET) == 0);
    assert(vfs_read(first, buf, 32) == 5 && memcmp(buf, "1234\n", 5) == 0);
    assert(vfs_lseek(first, -1, SEEK_SET) < 0);
    assert(vfs_lseek(first, 100, SEEK_SET) < 0);
    assert(vfs_close(first) == 0 && vfs_close(second) == 0);
    uptime_ms = UINT32_MAX;
    k_tick();
    assert(k_uptime_ms() == UINT64_C(4294967296));
    uptime_ms = UINT64_MAX;
    first = vfs_open("/dev/uptime", O_RDONLY);
    memset(buf, 0, sizeof(buf));
    assert(vfs_read(first, buf, sizeof(buf)) == 21);
    assert(strcmp(buf, "18446744073709551615\n") == 0);
    assert(vfs_close(first) == 0);
    puts("PASS: uptime, interrupt state, rollover, independent snapshots, partial reads, EOF, "
         "seek, read-only");
}
