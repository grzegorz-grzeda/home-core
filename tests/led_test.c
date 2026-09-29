/* SPDX-License-Identifier: MIT */
/* LED numbering, API, and /dev files over a fake GPIO port and a console LED. */
#include "console_led.h"
#include "gpio_led.h"
#include "homecore/autoconf.h"
#include "homecore/drivers/led.h"
#include "homecore/vfs/vfs.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

/* Fake GPIO port with 16 pins; an output reads back the level it drives. */
static bool levels[16];
static bool outputs[16];
static unsigned sets;

static int fake_configure(void *port, unsigned pin, gpio_mode_t mode) {
    (void)port;
    if (pin >= 16U) {
        errno = EINVAL;
        return -1;
    }
    outputs[pin] = mode == GPIO_OUTPUT;
    return 0;
}

static int fake_set(void *port, unsigned pin, bool high) {
    (void)port;
    if (pin >= 16U) {
        errno = EINVAL;
        return -1;
    }
    levels[pin] = high;
    sets++;
    return 0;
}

static int fake_get(void *port, unsigned pin, bool *high) {
    (void)port;
    if (pin >= 16U) {
        errno = EINVAL;
        return -1;
    }
    *high = levels[pin];
    return 0;
}

static const gpio_ops_t fake_ops = {.configure = fake_configure, .set = fake_set, .get = fake_get};

static const gpio_led_config_t high_config = {
    .port = {.ops = &fake_ops, .port = NULL}, .pin = 12, .path = "/dev/led0"};
static const gpio_led_config_t low_config = {
    .port = {.ops = &fake_ops, .port = NULL}, .pin = 3, .active_low = true, .path = "/dev/led1"};
static const console_led_config_t console_config = {.path = "/dev/led2"};
static gpio_led_t high_led = {.config = &high_config};
static gpio_led_t low_led = {.config = &low_config};
static console_led_t console_led = {.config = &console_config};

static const led_t leds[] = {
    {.ops = &gpio_led_led_ops, .device = &high_led},
    {.ops = &gpio_led_led_ops, .device = &low_led},
    {.ops = &console_led_led_ops, .device = &console_led},
};
const led_table_t dt_led_table = {.leds = leds, .count = 3};

static jmp_buf panic_return;
void board_panic(const char *message) {
    assert(strcmp(message, "gpio-led: invalid pin") == 0);
    longjmp(panic_return, 1);
}

static void read_file(const char *path, const char *expected) {
    int fd = vfs_open(path, O_RDONLY);
    assert(fd >= 0);
    char buffer[8] = {0};
    assert(vfs_read(fd, buffer, sizeof(buffer)) == (int)strlen(expected));
    assert(strcmp(buffer, expected) == 0);
    assert(vfs_read(fd, buffer, sizeof(buffer)) == 0); /* End of file. */
    assert(vfs_close(fd) == 0);
}

static int write_file(const char *path, const char *text) {
    int fd = vfs_open(path, O_WRONLY | O_CREAT | O_TRUNC);
    assert(fd >= 0);
    int result = vfs_write(fd, text, (unsigned)strlen(text));
    int error = errno;
    assert(vfs_close(fd) == 0);
    errno = error;
    return result;
}

static void check_init(void) {
    gpio_led_init(&high_led);
    gpio_led_init(&low_led);
    console_led_init(&console_led);
    /* Outputs start with the LED off: low when active-high, high when
     * active-low. */
    assert(outputs[12] && !levels[12]);
    assert(outputs[3] && levels[3]);

    static const gpio_led_config_t bad_config = {
        .port = {.ops = &fake_ops, .port = NULL}, .pin = 16, .path = "/dev/bad"};
    gpio_led_t bad = {.config = &bad_config};
    if (setjmp(panic_return) == 0) {
        gpio_led_init(&bad);
        assert(!"invalid pin must panic");
    }
    assert(!vfs_find_node("/dev/bad"));
}

static void check_api(void) {
    bool on = true;
    assert(led_count() == 3);
    assert(led_get(0, &on) == 0 && !on);
    assert(led_set(0, true) == 0 && levels[12]);
    assert(led_get(0, &on) == 0 && on);
    assert(led_set(1, true) == 0 && !levels[3]); /* Active-low: pin low. */
    assert(led_get(1, &on) == 0 && on);
    assert(led_toggle(1) == 0 && levels[3]);
    assert(led_set(2, true) == 0 && console_led.on);
    assert(led_toggle(2) == 0 && !console_led.on);
    errno = 0;
    assert(led_set(3, true) == -1 && errno == ENODEV);
    assert(led_get(3, &on) == -1 && errno == ENODEV);
    assert(led_toggle(3) == -1 && errno == ENODEV);
    assert(led_set(0, false) == 0 && led_set(1, false) == 0);
}

static void check_files(void) {
    read_file("/dev/led0", "0\n");
    assert(write_file("/dev/led0", "1") == 1 && levels[12]);
    read_file("/dev/led0", "1\n");
    assert(write_file("/dev/led0", "off\n") == 4 && !levels[12]);
    assert(write_file("/dev/led0", "  on \r\n") == 7 && levels[12]);
    assert(write_file("/dev/led0", "toggle") == 6 && !levels[12]);
    assert(write_file("/dev/led1", "on") == 2 && !levels[3]);
    read_file("/dev/led1", "1\n");
    assert(write_file("/dev/led2", "1\n") == 2 && console_led.on);
    read_file("/dev/led2", "1\n");

    unsigned before = sets;
    const char *invalid[] = {"", "  \n", "2", "onn", "blink", "toggle toggle toggle"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
        errno = 0;
        int fd = vfs_open("/dev/led0", O_WRONLY);
        int result = vfs_write(fd, invalid[i], (unsigned)strlen(invalid[i]));
        /* A zero-length write succeeds without reaching the driver. */
        assert(invalid[i][0] == '\0' ? result == 0 : (result == -1 && errno == EINVAL));
        assert(vfs_close(fd) == 0);
    }
    assert(sets == before);

    /* The snapshot is taken at open; a read-write descriptor keeps it. */
    int fd = vfs_open("/dev/led0", O_RDWR);
    assert(fd >= 0 && vfs_write(fd, "on", 2) == 2);
    char buffer[4] = {0};
    assert(vfs_read(fd, buffer, sizeof(buffer)) == 2 && strcmp(buffer, "0\n") == 0);
    assert(vfs_close(fd) == 0);
    read_file("/dev/led0", "1\n");
    int reader = vfs_open("/dev/led0", O_RDONLY);
    assert(vfs_write(reader, "on", 2) == -1 && errno == EBADF);
    assert(vfs_close(reader) == 0);
    assert(vfs_unlink("/dev/led0") == -1 && errno == EPERM);
}

int main(void) {
    check_init();
    check_api();
    check_files();
    puts("PASS: LED numbering, API, active-low, console LED, /dev files and errors");
    return 0;
}
