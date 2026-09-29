// SPDX-License-Identifier: MIT
#include "gpio_led.h"
#include "homecore/board/board.h"

static int gpio_led_set(void *device, bool on) {
    const gpio_led_config_t *config = ((const gpio_led_t *)device)->config;
    return config->port.ops->set(config->port.port, config->pin, on != config->active_low);
}

static int gpio_led_get(void *device, bool *on) {
    const gpio_led_config_t *config = ((const gpio_led_t *)device)->config;
    bool high;
    if (config->port.ops->get(config->port.port, config->pin, &high) < 0) {
        return -1;
    }
    *on = high != config->active_low;
    return 0;
}

const led_ops_t gpio_led_led_ops = {
    .set = gpio_led_set,
    .get = gpio_led_get,
};

void gpio_led_init(gpio_led_t *dev) {
    const gpio_led_config_t *config = dev->config;
    /* Drive the "off" level before enabling the output, so the LED does not
     * flash at startup. */
    if (config->port.ops->set(config->port.port, config->pin, config->active_low) < 0 ||
        config->port.ops->configure(config->port.port, config->pin, GPIO_OUTPUT) < 0) {
        board_panic("gpio-led: invalid pin");
    }
    dev->led = (led_t){.ops = &gpio_led_led_ops, .device = dev};
    led_register_node(&dev->node, config->path, &dev->led);
}
