/* SPDX-License-Identifier: MIT */
/* Stand-in LED for emulators without LEDs: keeps the state in RAM and prints
 * each change on the console. */
#ifndef HOMECORE_DRIVERS_CONSOLE_LED_H
#define HOMECORE_DRIVERS_CONSOLE_LED_H

#include "homecore/drivers/led.h"
#include "homecore/vfs/vfs.h"
#include <stdbool.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    const char *path; /* Device file, such as "/dev/led0" */
} console_led_config_t;

/* Per-instance state (RAM). The generator sets config; the driver owns the rest. */
typedef struct {
    const console_led_config_t *config;
    led_t led;
    vfs_node_t node;
    bool on;
} console_led_t;

/* Register config->path with the LED off. Prints nothing: the console is not
 * open during dt_init(). */
void console_led_init(console_led_t *dev);

extern const led_ops_t console_led_led_ops;

#endif /* HOMECORE_DRIVERS_CONSOLE_LED_H */
