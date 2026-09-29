// SPDX-License-Identifier: MIT
/* LED numbering, the led_* API, and the /dev/ledN files shared by every LED
 * driver. */
#include "homecore/drivers/led.h"
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

/* Longest accepted write, "toggle" with surrounding whitespace such as the
 * newline the shell's write command adds. */
#define LED_COMMAND_CAPACITY 16U

static const led_t *find(unsigned index) {
    if (index >= dt_led_table.count) {
        errno = ENODEV;
        return NULL;
    }
    return &dt_led_table.leds[index];
}

unsigned led_count(void) {
    return dt_led_table.count;
}

int led_set(unsigned index, bool on) {
    const led_t *led = find(index);
    return led ? led->ops->set(led->device, on) : -1;
}

int led_get(unsigned index, bool *on) {
    const led_t *led = find(index);
    return led ? led->ops->get(led->device, on) : -1;
}

static int toggle(const led_t *led) {
    bool on;
    if (led->ops->get(led->device, &on) < 0) {
        return -1;
    }
    return led->ops->set(led->device, !on);
}

int led_toggle(unsigned index) {
    const led_t *led = find(index);
    return led ? toggle(led) : -1;
}

static int led_snapshot(vfs_node_t *node, char *buf, unsigned capacity) {
    const led_t *led = node->driver_data;
    bool on;
    if (capacity < 2U || led->ops->get(led->device, &on) < 0) {
        return -1;
    }
    buf[0] = on ? '1' : '0';
    buf[1] = '\n';
    return 2;
}

/* Parse one command; the VFS has checked len and buf. */
static int led_write(vfs_node_t *node, const void *buf, unsigned len) {
    const led_t *led = node->driver_data;
    const char *text = buf;
    unsigned start = 0;
    unsigned end = len;
    while (start < end && isspace((unsigned char)text[start])) {
        start++;
    }
    while (end > start && isspace((unsigned char)text[end - 1U])) {
        end--;
    }
    char command[LED_COMMAND_CAPACITY];
    unsigned length = end - start;
    if (length == 0U || length >= sizeof(command) || len > INT_MAX) {
        errno = EINVAL;
        return -1;
    }
    memcpy(command, text + start, length);
    command[length] = '\0';
    int result;
    if (strcmp(command, "1") == 0 || strcmp(command, "on") == 0) {
        result = led->ops->set(led->device, true);
    } else if (strcmp(command, "0") == 0 || strcmp(command, "off") == 0) {
        result = led->ops->set(led->device, false);
    } else if (strcmp(command, "toggle") == 0) {
        result = toggle(led);
    } else {
        errno = EINVAL;
        return -1;
    }
    return result < 0 ? -1 : (int)len;
}

void led_register_node(vfs_node_t *node, const char *path, const led_t *led) {
    *node = (vfs_node_t){
        .name = path,
        .ops = {.snapshot = led_snapshot, .write = led_write},
        /* driver_data is non-const; led_snapshot() and led_write() only
         * read the LED through it. */
        .driver_data = (void *)led,
    };
    vfs_register_node(node);
}
