/* SPDX-License-Identifier: MIT */
/* STM32F4 GPIO port. */
#ifndef HOMECORE_DRIVERS_STM32F4_GPIO_H
#define HOMECORE_DRIVERS_STM32F4_GPIO_H

#include "homecore/drivers/gpio.h"
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    uintptr_t base;     /* GPIO register block address */
    uint32_t clock_bit; /* Port's enable bit in RCC->AHB1ENR */
} stm32f4_gpio_config_t;

/* Per-instance state (RAM). The port keeps nothing beyond its config. */
typedef struct {
    const stm32f4_gpio_config_t *config;
} stm32f4_gpio_t;

/* Enable the port clock. Pins keep their reset configuration until
 * configured. */
void stm32f4_gpio_init(stm32f4_gpio_t *dev);

extern const gpio_ops_t stm32f4_gpio_gpio_ops;

#endif /* HOMECORE_DRIVERS_STM32F4_GPIO_H */
