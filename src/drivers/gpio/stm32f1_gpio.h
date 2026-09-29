/* SPDX-License-Identifier: MIT */
/* STM32F1 GPIO port. */
#ifndef HOMECORE_DRIVERS_STM32F1_GPIO_H
#define HOMECORE_DRIVERS_STM32F1_GPIO_H

#include "homecore/drivers/gpio.h"
#include <stdint.h>

/* Per-instance constants generated from the device description (flash). */
typedef struct {
    uintptr_t base;     /* GPIO register block address */
    uint32_t clock_bit; /* Port's enable bit in RCC->APB2ENR */
} stm32f1_gpio_config_t;

/* Per-instance state (RAM). The port keeps nothing beyond its config. */
typedef struct {
    const stm32f1_gpio_config_t *config;
} stm32f1_gpio_t;

/* Enable the port clock. Pins keep their reset configuration until
 * configured. */
void stm32f1_gpio_init(stm32f1_gpio_t *dev);

extern const gpio_ops_t stm32f1_gpio_gpio_ops;

#endif /* HOMECORE_DRIVERS_STM32F1_GPIO_H */
