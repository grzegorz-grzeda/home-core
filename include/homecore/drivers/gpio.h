/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief General-purpose I/O ports described by the device description.
 */
#ifndef HOMECORE_DRIVERS_GPIO_H
#define HOMECORE_DRIVERS_GPIO_H

#include <stdbool.h>

/**
 * @defgroup gpio GPIO
 * @ingroup drivers
 * @brief Pin configuration, output, and input on GPIO ports.
 *
 * Each GPIO port is a device in `soc.yaml`, enabled by the boards that use it.
 * Other drivers refer to a port through a `gpio` property and receive a
 * ::gpio_port_t, which pairs the port driver's operations with the port
 * instance. The port driver enables the port's clock in its init function.
 *
 * Operations are for thread context: they read-modify-write configuration
 * registers without masking interrupts, so interrupt handlers must not
 * configure pins. Setting an output is a single register write on every
 * current port driver.
 *
 * | Driver | Compatible | Pins per port |
 * | --- | --- | --- |
 * | STM32F4 | `st,stm32f4-gpio` | 16 |
 * | STM32F1 | `st,stm32f1-gpio` | 16 |
 * @see @ref driver_gpio for the drivers, their configuration, and tests.
 * @{
 */

/** @brief Pin configuration for gpio_ops_t::configure. */
typedef enum {
    /** Floating input. */
    GPIO_INPUT,
    /** Input with the internal pull-up resistor. */
    GPIO_INPUT_PULL_UP,
    /** Input with the internal pull-down resistor. */
    GPIO_INPUT_PULL_DOWN,
    /** Push-pull output at the lowest speed, initially low. */
    GPIO_OUTPUT,
} gpio_mode_t;

/** @brief Operations a GPIO port driver provides; every one is required. */
typedef struct {
    /** Configure @p pin; returns 0, or -1 with `errno` `EINVAL` for an
     *  invalid pin or mode. */
    int (*configure)(void *port, unsigned pin, gpio_mode_t mode);
    /** Drive output @p pin high (`true`) or low; returns 0, or -1 with
     *  `errno` `EINVAL` for an invalid pin. */
    int (*set)(void *port, unsigned pin, bool high);
    /** Read the level of @p pin into @p high; returns 0, or -1 with `errno`
     *  `EINVAL` for an invalid pin. On the current ports, an output pin reads
     *  back the level it drives. */
    int (*get)(void *port, unsigned pin, bool *high);
} gpio_ops_t;

/** @brief A GPIO port: driver operations bound to one port instance. */
typedef struct {
    /** Operations of the port's driver. */
    const gpio_ops_t *ops;
    /** Port instance passed to every operation. */
    void *port;
} gpio_port_t;

/** @} */

#endif /* HOMECORE_DRIVERS_GPIO_H */
