// SPDX-License-Identifier: MIT
#include "soc_cmsis.h"

void Default_IRQHandler(void) {
    __disable_irq();
    for (;;) {
        __WFI();
    }
}

void WWDG_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void PVD_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TAMPER_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void RTC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel6_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Channel7_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void ADC1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_BRK_TIM15_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_UP_TIM16_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_TRG_COM_TIM17_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_CC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void I2C1_EV_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void I2C1_ER_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void I2C2_EV_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void I2C2_ER_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void SPI1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void SPI2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void USART3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI15_10_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void RTC_Alarm_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CEC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM6_DAC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM7_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));

/* STM32F100xB peripheral vectors 0-55; zero entries are reserved slots. */
__attribute__((used, section(".isr_vector.soc"), aligned(4))) void (*const soc_vectors[])(void) = {
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMPER_IRQHandler,
    RTC_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Channel1_IRQHandler,
    DMA1_Channel2_IRQHandler,
    DMA1_Channel3_IRQHandler,
    DMA1_Channel4_IRQHandler,
    DMA1_Channel5_IRQHandler,
    DMA1_Channel6_IRQHandler,
    DMA1_Channel7_IRQHandler,
    ADC1_IRQHandler,
    0,
    0,
    0,
    0,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM15_IRQHandler,
    TIM1_UP_TIM16_IRQHandler,
    TIM1_TRG_COM_TIM17_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    USART3_IRQHandler,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    CEC_IRQHandler,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
};
