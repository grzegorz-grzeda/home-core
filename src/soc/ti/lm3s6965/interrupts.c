// SPDX-License-Identifier: MIT
#include "homecore/arch/arch.h"

/* LM3S6965 peripheral vectors 0-45, generated from the CMSIS IRQn_Type enum.
 * Every implemented interrupt enters arch_irq_entry(), which dispatches to the
 * owning device through the device description. Zero entries are reserved. */
__attribute__((used, section(".isr_vector.soc"), aligned(4))) void (*const soc_vectors[])(void) = {
    arch_irq_entry, /*  0 GPIOA */
    arch_irq_entry, /*  1 GPIOB */
    arch_irq_entry, /*  2 GPIOC */
    arch_irq_entry, /*  3 GPIOD */
    arch_irq_entry, /*  4 GPIOE */
    arch_irq_entry, /*  5 UART0 */
    arch_irq_entry, /*  6 UART1 */
    arch_irq_entry, /*  7 SSI0 */
    arch_irq_entry, /*  8 I2C0 */
    arch_irq_entry, /*  9 PWM0_FAULT */
    arch_irq_entry, /* 10 PWM0_0 */
    arch_irq_entry, /* 11 PWM0_1 */
    arch_irq_entry, /* 12 PWM0_2 */
    arch_irq_entry, /* 13 QEI0 */
    arch_irq_entry, /* 14 ADC0SS0 */
    arch_irq_entry, /* 15 ADC0SS1 */
    arch_irq_entry, /* 16 ADC0SS2 */
    arch_irq_entry, /* 17 ADC0SS3 */
    arch_irq_entry, /* 18 WATCHDOG0 */
    arch_irq_entry, /* 19 TIMER0A */
    arch_irq_entry, /* 20 TIMER0B */
    arch_irq_entry, /* 21 TIMER1A */
    arch_irq_entry, /* 22 TIMER1B */
    arch_irq_entry, /* 23 TIMER2A */
    arch_irq_entry, /* 24 TIMER2B */
    arch_irq_entry, /* 25 COMP0 */
    arch_irq_entry, /* 26 COMP1 */
    arch_irq_entry, /* 27 COMP2 */
    arch_irq_entry, /* 28 SYSCTL */
    arch_irq_entry, /* 29 FLASH_CTRL */
    arch_irq_entry, /* 30 GPIOF */
    arch_irq_entry, /* 31 GPIOG */
    0,              /* 32 reserved */
    arch_irq_entry, /* 33 UART2 */
    0,              /* 34 reserved */
    arch_irq_entry, /* 35 TIMER3A */
    arch_irq_entry, /* 36 TIMER3B */
    arch_irq_entry, /* 37 I2C1 */
    arch_irq_entry, /* 38 QEI1 */
    0,              /* 39 reserved */
    0,              /* 40 reserved */
    0,              /* 41 reserved */
    arch_irq_entry, /* 42 ETH */
    arch_irq_entry, /* 43 HIB */
    0,              /* 44 reserved */
    arch_irq_entry, /* 45 PWM0_3 */
};
