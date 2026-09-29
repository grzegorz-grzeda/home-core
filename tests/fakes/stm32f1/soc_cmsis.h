/* SPDX-License-Identifier: MIT */
/* Host-test stand-in for the STM32F1 device header: the GPIO and RCC
 * registers used by src/drivers/gpio/stm32f1_gpio.c as plain memory. */
#ifndef HOMECORE_TEST_STM32F1_SOC_CMSIS_H
#define HOMECORE_TEST_STM32F1_SOC_CMSIS_H
#include <stdint.h>
typedef struct {
    volatile uint32_t CRL;
    volatile uint32_t CRH;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
} GPIO_TypeDef;
typedef struct {
    volatile uint32_t APB2ENR;
} RCC_TypeDef;
extern RCC_TypeDef test_rcc;
#define RCC (&test_rcc)
#endif
