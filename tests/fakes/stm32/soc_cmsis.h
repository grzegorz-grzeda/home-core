/* SPDX-License-Identifier: MIT */
/* Host-test stand-in for the STM32 device header: the USART registers used by
 * src/drivers/serial/stm32_usart.c as plain memory, with the real bit values. */
#ifndef HOMECORE_TEST_STM32_SOC_CMSIS_H
#define HOMECORE_TEST_STM32_SOC_CMSIS_H
#include <stdint.h>
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
} USART_TypeDef;
#define USART_SR_PE   (1U << 0)
#define USART_SR_FE   (1U << 1)
#define USART_SR_NE   (1U << 2)
#define USART_SR_ORE  (1U << 3)
#define USART_SR_RXNE (1U << 5)
#define USART_SR_TC   (1U << 6)
#define USART_SR_TXE  (1U << 7)
#define USART_CR1_RE  (1U << 2)
#define USART_CR1_TE  (1U << 3)
#define USART_CR1_UE  (1U << 13)
#endif
