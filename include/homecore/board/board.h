/*
 * MIT License
 *
 * Copyright (c) 2026 Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*---------------------------------------------------------------------------*/
/**
 * @file
 * @brief Board interface: clocks, pins, the polling console UART, and the
 *        fatal panic path.
 */
/*---------------------------------------------------------------------------*/
#ifndef HOME_CORE_BOARD_H
#define HOME_CORE_BOARD_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * @defgroup board Board support
 * @ingroup hal
 * @brief Board-specific clock policy, pin wiring, console, and panic output.
 *
 * Each board implements this interface in `src/board/<name>/board.c`. The
 * board owns the console UART that the kernel opens as `/dev/uart0`.
 * @{
 */
/*---------------------------------------------------------------------------*/
/** @brief Stringify @p x without macro-expanding it first. */
#define BOARD_STRINGIFY(x) #x
/**
 * @brief Intended to hold the board name as a string.
 *
 * @warning Unused. BOARD_STRINGIFY() does not expand its argument, so this
 *          macro yields `"HOMECORE_BOARD_NAME"` rather than the board name.
 */
#define BOARD BOARD_STRINGIFY(HOMECORE_BOARD_NAME)
/*---------------------------------------------------------------------------*/
/**
 * @brief Initialize board clocks, pins, and the console UART.
 *
 * `main()` calls it once, after soc_init() and before k_init(). The board
 * may call board_panic() if clock setup fails.
 */
void board_init(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Report the core clock frequency.
 *
 * @return Core clock in Hz, which also drives the system timer. It must match
 *         the clock configured by board_init().
 */
uint32_t board_cpu_clock_hz(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Write one byte to the console UART by polling.
 *
 * May block indefinitely. Call from foreground code or the fatal panic path.
 *
 * @param c Byte to transmit.
 */
void board_uart_putc(char c);
/*---------------------------------------------------------------------------*/
/**
 * @brief Read one byte from the console UART, blocking until one arrives.
 *
 * Call from foreground code only. There is no end-of-file condition.
 *
 * @return Received byte in the range 0 to 255.
 */
int board_uart_getc(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Check without blocking whether the console UART has received data.
 *
 * @pre board_init() has run.
 *
 * @return Nonzero if a byte is available, otherwise 0.
 */
int board_uart_has_data(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Report a fatal error and halt.
 *
 * Callable from fault handlers. Output is best effort: an early fault may halt
 * silently if the console is not yet initialized. Does not return on hardware.
 *
 * @param msg NUL-terminated message. The pointer is not retained.
 */
void board_panic(const char *msg);
/*---------------------------------------------------------------------------*/
/** @} */
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_BOARD_H
/*---------------------------------------------------------------------------*/
