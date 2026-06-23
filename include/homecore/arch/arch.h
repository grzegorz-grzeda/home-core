/**
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
#ifndef HOME_CORE_ARCH_H
#define HOME_CORE_ARCH_H
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
extern "C" {
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
/*---------------------------------------------------------------------------*/
typedef uintptr_t arch_irq_key_t;
/*---------------------------------------------------------------------------*/
void arch_init(void);
/*---------------------------------------------------------------------------*/
arch_irq_key_t arch_irq_lock(void);
/*---------------------------------------------------------------------------*/
void arch_irq_unlock(arch_irq_key_t key);
/*---------------------------------------------------------------------------*/
void arch_irq_enable(int irq);
/*---------------------------------------------------------------------------*/
void arch_irq_disable(int irq);
/*---------------------------------------------------------------------------*/
void arch_irq_set_priority(int irq, int priority);
/*---------------------------------------------------------------------------*/
void arch_irq_clear_pending(int irq);
/*---------------------------------------------------------------------------*/
void arch_irq_set_pending(int irq);
/*---------------------------------------------------------------------------*/
void arch_cpu_idle(void);
/*---------------------------------------------------------------------------*/
void arch_cpu_halt(void);
/*---------------------------------------------------------------------------*/
void arch_cpu_reset(void);
/*---------------------------------------------------------------------------*/
void arch_context_switch(void **old_sp, void *new_sp);
/*---------------------------------------------------------------------------*/
void arch_context_switch_to(void *new_sp) __attribute__((noreturn));
/*---------------------------------------------------------------------------*/
void arch_yield(void);
/*---------------------------------------------------------------------------*/
int arch_cpu_timer_init(uint32_t cpu_hz, uint32_t tick_hz);
/*---------------------------------------------------------------------------*/
void arch_cpu_timer_stop(void);
/*---------------------------------------------------------------------------*/
void arch_panic(void) __attribute__((noreturn));
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_ARCH_H
/*---------------------------------------------------------------------------*/
