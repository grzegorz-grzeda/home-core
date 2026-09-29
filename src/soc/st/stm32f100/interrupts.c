// SPDX-License-Identifier: MIT
#include "homecore/arch/arch.h"

/* STM32F100xB peripheral vectors 0-55, generated from the CMSIS IRQn_Type enum.
 * Every implemented interrupt enters arch_irq_entry(), which dispatches to the
 * owning device through the device description. Zero entries are reserved. */
__attribute__((used, section(".isr_vector.soc"), aligned(4))) void (*const soc_vectors[])(void) = {
    arch_irq_entry, /*  0 WWDG */
    arch_irq_entry, /*  1 PVD */
    arch_irq_entry, /*  2 TAMPER */
    arch_irq_entry, /*  3 RTC */
    arch_irq_entry, /*  4 FLASH */
    arch_irq_entry, /*  5 RCC */
    arch_irq_entry, /*  6 EXTI0 */
    arch_irq_entry, /*  7 EXTI1 */
    arch_irq_entry, /*  8 EXTI2 */
    arch_irq_entry, /*  9 EXTI3 */
    arch_irq_entry, /* 10 EXTI4 */
    arch_irq_entry, /* 11 DMA1_Channel1 */
    arch_irq_entry, /* 12 DMA1_Channel2 */
    arch_irq_entry, /* 13 DMA1_Channel3 */
    arch_irq_entry, /* 14 DMA1_Channel4 */
    arch_irq_entry, /* 15 DMA1_Channel5 */
    arch_irq_entry, /* 16 DMA1_Channel6 */
    arch_irq_entry, /* 17 DMA1_Channel7 */
    arch_irq_entry, /* 18 ADC1 */
    0,              /* 19 reserved */
    0,              /* 20 reserved */
    0,              /* 21 reserved */
    0,              /* 22 reserved */
    arch_irq_entry, /* 23 EXTI9_5 */
    arch_irq_entry, /* 24 TIM1_BRK_TIM15 */
    arch_irq_entry, /* 25 TIM1_UP_TIM16 */
    arch_irq_entry, /* 26 TIM1_TRG_COM_TIM17 */
    arch_irq_entry, /* 27 TIM1_CC */
    arch_irq_entry, /* 28 TIM2 */
    arch_irq_entry, /* 29 TIM3 */
    arch_irq_entry, /* 30 TIM4 */
    arch_irq_entry, /* 31 I2C1_EV */
    arch_irq_entry, /* 32 I2C1_ER */
    arch_irq_entry, /* 33 I2C2_EV */
    arch_irq_entry, /* 34 I2C2_ER */
    arch_irq_entry, /* 35 SPI1 */
    arch_irq_entry, /* 36 SPI2 */
    arch_irq_entry, /* 37 USART1 */
    arch_irq_entry, /* 38 USART2 */
    arch_irq_entry, /* 39 USART3 */
    arch_irq_entry, /* 40 EXTI15_10 */
    arch_irq_entry, /* 41 RTC_Alarm */
    arch_irq_entry, /* 42 CEC */
    0,              /* 43 reserved */
    0,              /* 44 reserved */
    0,              /* 45 reserved */
    0,              /* 46 reserved */
    0,              /* 47 reserved */
    0,              /* 48 reserved */
    0,              /* 49 reserved */
    0,              /* 50 reserved */
    0,              /* 51 reserved */
    0,              /* 52 reserved */
    0,              /* 53 reserved */
    arch_irq_entry, /* 54 TIM6_DAC */
    arch_irq_entry, /* 55 TIM7 */
};
