/* SPDX-License-Identifier: MIT */
/* Host-test stand-in for the STM32F4 device header: the GPIO and RCC
 * registers used by src/drivers/gpio/stm32f4_gpio.c as plain memory. */
#ifndef HOMECORE_TEST_STM32F4_SOC_CMSIS_H
#define HOMECORE_TEST_STM32F4_SOC_CMSIS_H
#include <stdint.h>
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
} GPIO_TypeDef;
typedef struct {
    volatile uint32_t AHB1ENR;
} RCC_TypeDef;
extern RCC_TypeDef test_rcc;
#define RCC (&test_rcc)
#endif
