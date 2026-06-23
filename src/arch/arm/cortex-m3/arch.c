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
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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
#include "homecore/arch/arch.h"
#include "soc_cmsis.h"
#include "core_cm3.h"
/*---------------------------------------------------------------------------*/
#define INITIAL_XPSR 0x01000000u
/*---------------------------------------------------------------------------*/
struct arch_exception_frame {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
};
/*---------------------------------------------------------------------------*/
struct arch_callee_saved_frame {
    uint32_t r4;
    uint32_t r5;
    uint32_t r6;
    uint32_t r7;
    uint32_t r8;
    uint32_t r9;
    uint32_t r10;
    uint32_t r11;
};
/*---------------------------------------------------------------------------*/
volatile void **arch_switch_old_sp;
/*---------------------------------------------------------------------------*/
volatile void *arch_switch_new_sp;
/*---------------------------------------------------------------------------*/
volatile void *arch_first_sp;
/*---------------------------------------------------------------------------*/
void arch_init(void) {
    NVIC_SetPriority(PendSV_IRQn, 0xFF);
    NVIC_SetPriority(SysTick_IRQn, 0xFE);
    NVIC_SetPriority(SVCall_IRQn, 0xFD);
}
/*---------------------------------------------------------------------------*/
arch_irq_key_t arch_irq_lock(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    __DSB();
    __ISB();
    return primask;
}
/*---------------------------------------------------------------------------*/
void arch_irq_unlock(arch_irq_key_t key) {
    if ((key & 1U) == 0U) {
        __enable_irq();
    }
}
/*---------------------------------------------------------------------------*/
void arch_irq_enable(int irq) {
    NVIC_EnableIRQ((IRQn_Type)irq);
}
/*---------------------------------------------------------------------------*/
void arch_irq_disable(int irq) {
    NVIC_DisableIRQ((IRQn_Type)irq);
}
/*---------------------------------------------------------------------------*/
void arch_irq_set_priority(int irq, int priority) {
    NVIC_SetPriority((IRQn_Type)irq, priority);
}
/*---------------------------------------------------------------------------*/
void arch_irq_clear_pending(int irq) {
    NVIC_ClearPendingIRQ((IRQn_Type)irq);
}
/*---------------------------------------------------------------------------*/
void arch_irq_set_pending(int irq) {
    NVIC_SetPendingIRQ((IRQn_Type)irq);
}
/*---------------------------------------------------------------------------*/
void arch_cpu_idle(void) {
    __WFI();
}
/*---------------------------------------------------------------------------*/
void arch_cpu_halt(void) {
    arch_irq_lock();

    while (1) {
        __BKPT(0);
        __WFI();
    }
}
/*---------------------------------------------------------------------------*/
void arch_cpu_reset(void) {
    NVIC_SystemReset();
}
/*---------------------------------------------------------------------------*/
void *arch_stack_init(void *stack_mem,
                      size_t stack_size,
                      arch_thread_entry_t entry,
                      void *arg,
                      arch_thread_exit_t exit) {
    uintptr_t sp = (uintptr_t)stack_mem + stack_size;

    sp &= ~(uintptr_t)0x7; /* 8-byte alignment */

    struct arch_exception_frame *eframe;
    sp -= sizeof(struct arch_exception_frame);
    eframe = (struct arch_exception_frame *)sp;

    eframe->r0 = (uint32_t)arg;
    eframe->r1 = 0;
    eframe->r2 = 0;
    eframe->r3 = 0;
    eframe->r12 = 0;
    eframe->lr = (uint32_t)exit;
    eframe->pc = (uint32_t)entry;
    eframe->xpsr = INITIAL_XPSR;

    struct arch_callee_saved_frame *cframe;
    sp -= sizeof(struct arch_callee_saved_frame);
    cframe = (struct arch_callee_saved_frame *)sp;

    cframe->r4 = 0;
    cframe->r5 = 0;
    cframe->r6 = 0;
    cframe->r7 = 0;
    cframe->r8 = 0;
    cframe->r9 = 0;
    cframe->r10 = 0;
    cframe->r11 = 0;

    return (void *)sp;
}
/*---------------------------------------------------------------------------*/
void arch_context_switch(void **old_sp, void *new_sp) {
    arch_switch_old_sp = (volatile void **)old_sp;
    arch_switch_new_sp = new_sp;

    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

    __DSB();
    __ISB();
}
/*---------------------------------------------------------------------------*/
void arch_context_switch_to(void *new_sp) {
    arch_first_sp = new_sp;

    __asm volatile("svc #0");

    __builtin_unreachable();
}
/*---------------------------------------------------------------------------*/
void arch_yield(void) {
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}
/*---------------------------------------------------------------------------*/
int arch_cpu_timer_init(uint32_t cpu_hz, uint32_t tick_hz) {
    uint32_t reload = (cpu_hz / tick_hz) - 1U;

    if (reload > SysTick_LOAD_RELOAD_Msk) {
        return -1;
    }

    SysTick->LOAD = reload;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    return 0;
}
/*---------------------------------------------------------------------------*/
void arch_cpu_timer_stop(void) {
    SysTick->CTRL = 0;
}
/*---------------------------------------------------------------------------*/
void arch_panic(void) {
    arch_irq_lock();

    while (1) {
        __BKPT(0);
        __WFI();
    }
}
/*---------------------------------------------------------------------------*/
