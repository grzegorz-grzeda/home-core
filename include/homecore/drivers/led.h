/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief LEDs described by the device description, numbered from 0.
 */
#ifndef HOMECORE_DRIVERS_LED_H
#define HOMECORE_DRIVERS_LED_H

#include "homecore/vfs/vfs.h"
#include <stdbool.h>

/**
 * @defgroup led LEDs
 * @ingroup drivers
 * @brief Numbered LEDs, their `/dev/ledN` files, and the driver interface.
 *
 * Every enabled device whose binding has `led: true` is an LED. They are
 * numbered in description order, and each registers a device file, by default
 * `/dev/` plus its node name, so boards name the nodes `led0`, `led1`, and so
 * on. The generated `dt_led_table` lists them.
 *
 * | Driver | Compatible | Use |
 * | --- | --- | --- |
 * | GPIO LED | `homecore,gpio-led` | An LED on a GPIO pin, optionally active-low |
 * | Console LED | `homecore,console-led` | A stand-in for emulators without LEDs: prints each
 * change on the console |
 *
 * @section led_file Device files
 *
 * Reading a `/dev/ledN` file returns its state captured at open, `0\n` or
 * `1\n`, then end of file. Each write sets the LED: `1` or `on` lights it, `0`
 * or `off` turns it off, and `toggle` inverts it. Surrounding whitespace, such
 * as a trailing newline, is ignored; anything else fails with `EINVAL`. The
 * file accepts `O_CREAT`, `O_TRUNC`, and `O_APPEND`, so `fopen(path, "w")`
 * works.
 *
 * @code{.sh}
 * write /dev/led0 on
 * cat /dev/led0
 * @endcode
 *
 * The functions below are for thread context, like the VFS.
 * @see @ref driver_led for the drivers, their configuration, and tests.
 * @{
 */

/** @name Using LEDs
 * @{
 */
/**
 * @brief Report how many LEDs the board describes.
 *
 * @return The number of LEDs; valid indices are 0 to one less.
 */
unsigned led_count(void);

/**
 * @brief Turn an LED on or off.
 *
 * @param index LED number.
 * @param on    `true` to light the LED.
 *
 * @retval 0  The LED was set.
 * @retval -1 `errno` is `ENODEV` for an index without an LED, or set by the
 *            driver.
 */
int led_set(unsigned index, bool on);

/**
 * @brief Read whether an LED is lit.
 *
 * @param index   LED number.
 * @param[out] on `true` if the LED is lit. Must not be `NULL`.
 *
 * @retval 0  @p on holds the state.
 * @retval -1 `errno` is `ENODEV` for an index without an LED, or set by the
 *            driver.
 */
int led_get(unsigned index, bool *on);

/**
 * @brief Invert an LED.
 *
 * @param index LED number.
 *
 * @retval 0  The LED was inverted.
 * @retval -1 `errno` as for led_get() and led_set().
 */
int led_toggle(unsigned index);
/** @} */

/** @name Driver interface
 * @{
 */
/** @brief Operations an LED driver provides; every one is required. */
typedef struct {
    /** Light (`true`) or turn off the LED; returns 0 or -1 with `errno` set. */
    int (*set)(void *device, bool on);
    /** Store whether the LED is lit; returns 0 or -1 with `errno` set. */
    int (*get)(void *device, bool *on);
} led_ops_t;

/** @brief An LED: driver operations bound to one device instance. */
typedef struct {
    /** Operations of the LED's driver. */
    const led_ops_t *ops;
    /** Driver instance passed to every operation. */
    void *device;
} led_t;

/** @brief Every LED in index order; generated in `devicetree.c`. */
typedef struct {
    /** The LEDs, or `NULL` when @p count is 0. */
    const led_t *leds;
    /** Number of LEDs. */
    unsigned count;
} led_table_t;

/** @brief The board's LEDs; generated in `devicetree.c`. */
extern const led_table_t dt_led_table;

/**
 * @brief Register the device file of an LED.
 *
 * Called by LED drivers from their init function. Sets up @p node as a
 * readable and writable device file for @p led and registers it.
 *
 * @param node Node storage owned by the driver instance; must remain valid for
 *             the rest of the program.
 * @param path Device file path, such as `/dev/led0`; must remain valid.
 * @param led  The LED; must remain valid for the rest of the program.
 */
void led_register_node(vfs_node_t *node, const char *path, const led_t *led);
/** @} */

/** @} */

#endif /* HOMECORE_DRIVERS_LED_H */
