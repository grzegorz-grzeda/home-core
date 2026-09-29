/* SPDX-License-Identifier: MIT */
/* LED on a GPIO pin. */
#ifndef HOMECORE_DRIVERS_GPIO_LED_H
#define HOMECORE_DRIVERS_GPIO_LED_H

#include "homecore/drivers/gpio.h"
#include "homecore/drivers/led.h"
#include "homecore/vfs/vfs.h"
#include <stdbool.h>
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    gpio_port_t port; /* Port driving the LED */
    uint32_t pin;     /* Pin number within the port */
    bool active_low;  /* The LED lights when the pin is low */
    const char *path; /* Device file, such as "/dev/led0" */
} gpio_led_config_t;

/* Per-instance state (RAM). The generator sets config; the driver owns the rest. */
typedef struct {
    const gpio_led_config_t *config;
    led_t led;
    vfs_node_t node;
} gpio_led_t;

/* Configure the pin as an output with the LED off and register config->path.
 * The port must already be initialized; the generator orders dt_init() so.
 * Panics for a pin the port rejects. */
void gpio_led_init(gpio_led_t *dev);

extern const led_ops_t gpio_led_led_ops;

#endif /* HOMECORE_DRIVERS_GPIO_LED_H */
