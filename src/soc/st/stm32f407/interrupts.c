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
void TAMP_STAMP_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN1_TX_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN1_RX0_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN1_RX1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN1_SCE_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void EXTI9_5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_BRK_TIM9_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_UP_TIM10_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM1_TRG_COM_TIM11_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
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
void OTG_FS_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM8_BRK_TIM12_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM8_UP_TIM13_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM8_TRG_COM_TIM14_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM8_CC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA1_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void FSMC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void SDIO_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void SPI3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void UART4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void UART5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM6_DAC_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void TIM7_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void ETH_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void ETH_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN2_TX_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN2_RX0_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN2_RX1_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void CAN2_SCE_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void OTG_FS_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DMA2_Stream7_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void USART6_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void I2C3_EV_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void I2C3_ER_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void OTG_HS_EP1_OUT_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void OTG_HS_EP1_IN_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void OTG_HS_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void OTG_HS_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void DCMI_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void HASH_RNG_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));
void FPU_IRQHandler(void) __attribute__((weak, alias("Default_IRQHandler")));

/* IRQ slots 0..81, immediately after the 16 architectural vectors. */
__attribute__((used, section(".isr_vector.soc"), aligned(4))) void (*const soc_vectors[])(void) = {
    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    CAN1_TX_IRQHandler,
    CAN1_RX0_IRQHandler,
    CAN1_RX1_IRQHandler,
    CAN1_SCE_IRQHandler,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
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
    OTG_FS_WKUP_IRQHandler,
    TIM8_BRK_TIM12_IRQHandler,
    TIM8_UP_TIM13_IRQHandler,
    TIM8_TRG_COM_TIM14_IRQHandler,
    TIM8_CC_IRQHandler,
    DMA1_Stream7_IRQHandler,
    FSMC_IRQHandler,
    SDIO_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    UART4_IRQHandler,
    UART5_IRQHandler,
    TIM6_DAC_IRQHandler,
    TIM7_IRQHandler,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    ETH_IRQHandler,
    ETH_WKUP_IRQHandler,
    CAN2_TX_IRQHandler,
    CAN2_RX0_IRQHandler,
    CAN2_RX1_IRQHandler,
    CAN2_SCE_IRQHandler,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    OTG_HS_EP1_OUT_IRQHandler,
    OTG_HS_EP1_IN_IRQHandler,
    OTG_HS_WKUP_IRQHandler,
    OTG_HS_IRQHandler,
    DCMI_IRQHandler,
    0,
    HASH_RNG_IRQHandler,
    FPU_IRQHandler,
};
