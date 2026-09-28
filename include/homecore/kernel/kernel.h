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
 * @brief Kernel interface: initialization, uptime, and heap statistics.
 */
/*---------------------------------------------------------------------------*/
#ifndef HOME_CORE_KERNEL_H
#define HOME_CORE_KERNEL_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
/*---------------------------------------------------------------------------*/
/**
 * @defgroup kernel Kernel
 * @ingroup kernel_services
 * @brief Kernel startup, the millisecond uptime clock, and heap usage.
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * @brief Initialize kernel services and the standard streams.
 *
 * Starts the uptime clock (see k_uptime_init()) and opens `/dev/uart0` as
 * descriptors 0, 1, and 2 for stdin, stdout, and stderr. Calls board_panic()
 * if any step fails. `main()` calls it once, after soc_init() and
 * board_init().
 */
void k_init(void);
/*---------------------------------------------------------------------------*/
/** @name Uptime
 * @{
 */
/**
 * @brief Start the 1 kHz system timer and register `/dev/uptime`.
 *
 * Called by k_init(). Calls board_panic() if the timer cannot produce 1 kHz
 * from board_cpu_clock_hz().
 */
void k_uptime_init(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Read the uptime clock.
 *
 * Masks interrupts briefly to read the 64-bit counter consistently.
 * Masking interrupts for longer than one tick elsewhere loses time.
 *
 * @return Milliseconds since the system timer started during k_init().
 */
uint64_t k_uptime_ms(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Advance the uptime clock by one millisecond.
 *
 * Call only from the 1 kHz system timer interrupt.
 */
void k_tick(void);
/** @} */
/*---------------------------------------------------------------------------*/
/** @name Heap statistics
 * @{
 */
/** @brief Heap usage snapshot filled by k_heap_stats(). All values are bytes. */
typedef struct {
    /** Size of the heap region between static RAM and the reserved main stack. */
    size_t total;
    /** Bytes in allocated blocks, including allocator overhead. */
    size_t allocated;
    /** Free bytes the allocator already holds for reuse. */
    size_t reusable;
    /** Bytes of the heap region not yet claimed by the allocator. */
    size_t unclaimed;
} k_heap_stats_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Capture current heap usage.
 *
 * Available memory is `reusable + unclaimed`, which is not necessarily one
 * contiguous block.
 *
 * @param[out] stats Destination. Must not be `NULL`; this is not checked.
 */
void k_heap_stats(k_heap_stats_t *stats);
/** @} */
/*---------------------------------------------------------------------------*/
/** @name Stack statistics
 * @{
 */
/**
 * @brief Word written over the unused main stack at reset.
 *
 * The reset handler fills the main-stack reservation below its own frame with
 * this value. k_stack_stats() finds the deepest word that no longer holds it.
 */
#define K_STACK_FILL_WORD 0xA5A5A5A5U
/*---------------------------------------------------------------------------*/
/** @brief Main-stack usage filled by k_stack_stats(). All values are bytes. */
typedef struct {
    /** Size of the main-stack reservation, `CONFIG_HOMECORE_KERNEL_MAIN_STACK_SIZE`. */
    size_t size;
    /** Deepest stack use since reset, including interrupt handlers. */
    size_t used;
} k_stack_stats_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Measure the main stack's high-water mark.
 *
 * Scans the reservation from its lowest address for the first word that was
 * overwritten since reset. `used == size` means the whole reservation was used
 * and the stack may have overflowed into the heap, which is not detected
 * otherwise. The scan reads up to `size` bytes; call it from foreground code.
 *
 * @param[out] stats Destination. Must not be `NULL`; this is not checked.
 */
void k_stack_stats(k_stack_stats_t *stats);
/** @} */
/*---------------------------------------------------------------------------*/
/** @} */
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_KERNEL_H
/*---------------------------------------------------------------------------*/
