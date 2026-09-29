/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief Console device selected by the device description.
 */
#ifndef HOMECORE_DRIVERS_CONSOLE_H
#define HOMECORE_DRIVERS_CONSOLE_H

#include <stdbool.h>

/**
 * @defgroup console Console
 * @ingroup drivers
 * @brief Direct access, outside the VFS, to the device named by `chosen.console`.
 *
 * The generated device description defines #dt_console, which pairs a driver's
 * console operations with the chosen device instance. The board interface's
 * `board_uart_*` functions and panic paths use it, so boards contain no UART
 * code. Every operation is valid only after dt_init() has initialized the
 * device; check console_ready() on paths that can run earlier.
 * @{
 */

/** @brief Console operations a serial driver provides. */
typedef struct {
    /** Write one byte, polling while the transmitter is busy. Usable with interrupts masked. */
    void (*putc)(void *device, char c);
    /** Read one byte from the receive buffer, sleeping until one arrives; returns 0 to 255.
     * Needs interrupts enabled, because the receive interrupt fills the buffer. */
    int (*getc)(void *device);
    /** Return nonzero if a received byte is buffered. */
    int (*has_data)(void *device);
    /** Wait until every written byte has left the transmitter. */
    void (*flush)(void *device);
    /** Return true once the device is initialized. */
    bool (*ready)(void *device);
} console_ops_t;

/** @brief A console: driver operations bound to one device instance. */
typedef struct {
    /** Operations of the device's driver. */
    const console_ops_t *ops;
    /** Driver instance passed to every operation. */
    void *device;
} console_t;

/** @brief Console chosen by the device description; generated in `devicetree.c`. */
extern const console_t dt_console;

/**
 * @brief Report whether the console device has been initialized.
 *
 * @return `true` after dt_init() has initialized the chosen console device.
 */
bool console_ready(void);

/** @brief Wait until all written console output has been transmitted. */
void console_flush(void);

/** @} */

#endif /* HOMECORE_DRIVERS_CONSOLE_H */
