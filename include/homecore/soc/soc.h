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
 * @brief SoC interface: chip-level device registration.
 */
/*---------------------------------------------------------------------------*/
#ifndef HOME_CORE_SOC_H
#define HOME_CORE_SOC_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/**
 * @defgroup soc SoC
 * @ingroup hal
 * @brief Chip-specific device nodes and peripheral interrupt vectors.
 *
 * Each chip implements this interface in `src/soc/<vendor>/<chip>/soc.c` and
 * supplies its peripheral vector entries after the architecture's core vectors.
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * @brief Register the chip's device nodes with the VFS.
 *
 * Must register at least `/dev/uart0`, which k_init() opens as the standard
 * streams. `main()` calls it once, before board_init(). It must not access
 * board hardware that is still uninitialized, and it must not print.
 */
void soc_init(void);
/*---------------------------------------------------------------------------*/
/** @} */
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_SOC_H
/*---------------------------------------------------------------------------*/
