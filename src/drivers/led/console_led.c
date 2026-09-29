// SPDX-License-Identifier: MIT
#include "console_led.h"
#include <stdio.h>
#include <string.h>

/* Prints "[led0] on" for every set, including one that does not change the
 * state, so each request is visible. */
static int console_led_set(void *device, bool on) {
    console_led_t *dev = device;
    dev->on = on;
    const char *name = strrchr(dev->config->path, '/') + 1;
    return printf("[%s] %s\n", name, on ? "on" : "off") < 0 ? -1 : 0;
}

static int console_led_get(void *device, bool *on) {
    *on = ((const console_led_t *)device)->on;
    return 0;
}

const led_ops_t console_led_led_ops = {
    .set = console_led_set,
    .get = console_led_get,
};

void console_led_init(console_led_t *dev) {
    dev->on = false;
    dev->led = (led_t){.ops = &console_led_led_ops, .device = dev};
    led_register_node(&dev->node, dev->config->path, &dev->led);
}
