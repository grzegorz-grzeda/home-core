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
 * @brief Architecture interface: CPU control, interrupts, system timer, and
 *        thread-switch primitives.
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
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/**
 * @defgroup arch Architecture
 * @ingroup hal
 * @brief CPU-family mechanisms shared by every SoC using that CPU.
 *
 * The Cortex-M3 and Cortex-M4 ports share one implementation in
 * `src/arch/arm/cortex-m/`. Unless noted otherwise, call these functions from
 * foreground code.
 * @{
 */
/*---------------------------------------------------------------------------*/
/** @name CPU control
 * @{
 */
/**
 * @brief Point the vector table at the linked image and set core exception
 *        priorities.
 *
 * Sets VTOR to the start of the linked vector table. It then gives PendSV the
 * lowest priority, SysTick the next, and SVC the one above that. `main()` calls
 * it once, before any other initialization.
 */
void arch_init(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Wait for an interrupt.
 *
 * Returns after an interrupt becomes pending. It also returns while
 * interrupts are masked.
 */
void arch_cpu_idle(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Mask interrupts and stop permanently.
 *
 * Executes a breakpoint followed by wait-for-interrupt in a loop. With a
 * debugger attached, execution halts at the breakpoint. Without one, the
 * breakpoint escalates to HardFault, whose handler calls board_panic().
 */
void arch_cpu_halt(void);
/*---------------------------------------------------------------------------*/
/** @brief Request a system reset. Does not return on hardware. */
void arch_cpu_reset(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Halt permanently after a fatal error, without reporting a message.
 *
 * Behaves like arch_cpu_halt(). Prefer board_panic(), which reports a message
 * when the console is available.
 */
void arch_panic(void) __attribute__((noreturn));
/** @} */
/*---------------------------------------------------------------------------*/
/** @name Interrupt control
 * @{
 */
/** @brief Saved interrupt mask state returned by arch_irq_lock(). */
typedef uintptr_t arch_irq_key_t;
/*---------------------------------------------------------------------------*/
/**
 * @brief Mask all configurable-priority interrupts.
 *
 * Nested use is safe when each key is passed to arch_irq_unlock() in reverse
 * order. Callable from interrupt context.
 *
 * @return Mask state to pass to arch_irq_unlock().
 */
arch_irq_key_t arch_irq_lock(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Restore the interrupt mask saved by arch_irq_lock().
 *
 * Re-enables interrupts only if they were enabled when @p key was taken.
 *
 * @param key Value returned by the matching arch_irq_lock() call.
 */
void arch_irq_unlock(arch_irq_key_t key);
/*---------------------------------------------------------------------------*/
/**
 * @brief Report whether interrupts are masked.
 *
 * @warning Declared but not implemented by any port. A call fails to link.
 *
 * @return `true` if interrupts are masked.
 */
bool arch_irq_is_locked(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Common entry for every peripheral interrupt.
 *
 * The SoC vector tables point each implemented peripheral vector here. It
 * reads the active interrupt number and calls the generated
 * dt_irq_dispatch(), which runs the handler of the device that owns the
 * interrupt in the device description. Runs in interrupt context.
 */
void arch_irq_entry(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Enable a peripheral interrupt in the interrupt controller.
 *
 * @param irq Device IRQ number from the SoC's CMSIS header. Negative numbers
 *            select core exceptions, which this function ignores.
 */
void arch_irq_enable(int irq);
/*---------------------------------------------------------------------------*/
/**
 * @brief Disable a peripheral interrupt in the interrupt controller.
 *
 * @param irq Device IRQ number from the SoC's CMSIS header. Negative numbers
 *            select core exceptions, which this function ignores.
 */
void arch_irq_disable(int irq);
/*---------------------------------------------------------------------------*/
/**
 * @brief Set the priority of an interrupt or core exception.
 *
 * @param irq      IRQ number from the SoC's CMSIS header. Negative numbers
 *                 select configurable core exceptions.
 * @param priority Priority level. Lower values are more urgent. Only the
 *                 implemented priority bits (`__NVIC_PRIO_BITS`) are kept.
 */
void arch_irq_set_priority(int irq, int priority);
/*---------------------------------------------------------------------------*/
/**
 * @brief Clear a pending peripheral interrupt.
 *
 * @param irq Device IRQ number from the SoC's CMSIS header.
 */
void arch_irq_clear_pending(int irq);
/*---------------------------------------------------------------------------*/
/**
 * @brief Set a peripheral interrupt pending.
 *
 * @param irq Device IRQ number from the SoC's CMSIS header.
 */
void arch_irq_set_pending(int irq);
/** @} */
/*---------------------------------------------------------------------------*/
/** @name System timer
 * @{
 */
/**
 * @brief Start the periodic system timer.
 *
 * Starts SysTick from the core clock. Each tick interrupt calls k_tick().
 *
 * @param cpu_hz  Core clock frequency, normally board_cpu_clock_hz().
 * @param tick_hz Interrupt frequency. It must divide @p cpu_hz exactly.
 *
 * @retval 0  The timer is running.
 * @retval -1 @p tick_hz is zero, exceeds @p cpu_hz, or does not divide it
 *            exactly, or the reload value does not fit the 24-bit counter.
 */
int arch_cpu_timer_init(uint32_t cpu_hz, uint32_t tick_hz);
/*---------------------------------------------------------------------------*/
/** @brief Stop the system timer and its interrupt. */
void arch_cpu_timer_stop(void);
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * @name Thread primitives
 *
 * Scaffolding for a future scheduler. No code calls these functions yet. The
 * SVC and PendSV handlers they rely on currently call board_panic(), so
 * arch_context_switch(), arch_context_switch_to(), and arch_yield() end in a
 * fatal halt. Floating-point context is not preserved.
 * @{
 */
/**
 * @brief Thread entry point.
 *
 * @param arg Argument passed to arch_stack_init().
 */
typedef void (*arch_thread_entry_t)(void *arg);
/*---------------------------------------------------------------------------*/
/** @brief Function the thread runs if its entry point returns. */
typedef void (*arch_thread_exit_t)(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Build the initial stack frame for a new thread.
 *
 * The stack top is aligned down to 8 bytes. The function then writes an
 * exception frame followed by zeroed callee-saved registers, which takes
 * 64 bytes. The stack size is not validated.
 *
 * @param stack_mem  Lowest address of the thread stack.
 * @param stack_size Stack size in bytes.
 * @param entry      Thread entry point, loaded into the program counter.
 * @param arg        Argument passed to @p entry.
 * @param exit       Function run if @p entry returns.
 *
 * @return Initial stack pointer for arch_context_switch() or
 *         arch_context_switch_to().
 */
void *arch_stack_init(void *stack_mem,
                      size_t stack_size,
                      arch_thread_entry_t entry,
                      void *arg,
                      arch_thread_exit_t exit);
/*---------------------------------------------------------------------------*/
/**
 * @brief Request a switch from the current thread to another one.
 *
 * Records both stack pointers and sets PendSV pending.
 *
 * @param old_sp Where to save the current thread's stack pointer.
 * @param new_sp Stack pointer of the thread to resume.
 */
void arch_context_switch(void **old_sp, void *new_sp);
/*---------------------------------------------------------------------------*/
/**
 * @brief Start the first thread through a supervisor call.
 *
 * @param new_sp Stack pointer returned by arch_stack_init().
 */
void arch_context_switch_to(void *new_sp) __attribute__((noreturn));
/*---------------------------------------------------------------------------*/
/** @brief Set PendSV pending to request rescheduling. */
void arch_yield(void);
/** @} */
/*---------------------------------------------------------------------------*/
/** @} */
/*---------------------------------------------------------------------------*/
#if defined(__cplusplus)
}
#endif
/*---------------------------------------------------------------------------*/
#endif // HOME_CORE_ARCH_H
/*---------------------------------------------------------------------------*/
