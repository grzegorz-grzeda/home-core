// SPDX-License-Identifier: MIT
#include "homecore/board/board.h"
#include "soc_cmsis.h"
#include <stdbool.h>

#define CPU_HZ       16000000U
#define CONSOLE_BAUD 115200U
/* Iteration budget: startup has no timer yet; this is not a millisecond timeout. */
#define CLOCK_WAIT_LIMIT 1000000U
static bool uart_ready;

static bool wait_clock(volatile const uint32_t *reg, uint32_t mask, uint32_t value) {
    for (uint32_t attempt = 0; attempt < CLOCK_WAIT_LIMIT; ++attempt) {
        if ((*reg & mask) == value) {
            return true;
        }
    }
    return false;
}

uint32_t board_cpu_clock_hz(void) {
    return CPU_HZ;
}

void board_init(void) {
    /* Use HSI with undivided AHB/APB clocks; no crystal or PLL required. */
    RCC->CR |= RCC_CR_HSION;
    if (!wait_clock(&RCC->CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY)) {
        board_panic("HSI did not become ready");
        return;
    }
    RCC->CFGR &= ~RCC_CFGR_SW;
    if (!wait_clock(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_HSI)) {
        board_panic("Cannot switch system clock to HSI");
        return;
    }
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->AHB1ENR;
    (void)RCC->APB1ENR;
    RCC->APB1RSTR |= RCC_APB1RSTR_USART2RST;
    RCC->APB1RSTR &= ~RCC_APB1RSTR_USART2RST;

    /* PA2 = USART2_TX, PA3 = USART2_RX, alternate function 7. */
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 4) | (3U << 6))) | (2U << 4) | (2U << 6);
    GPIOA->OTYPER &= ~((1U << 2) | (1U << 3));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 4) | (3U << 6))) | (2U << 4) | (2U << 6);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~((3U << 4) | (3U << 6))) | (1U << 6);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((15U << 8) | (15U << 12))) | (7U << 8) | (7U << 12);

    /* 115200 baud, 8 data bits, no parity, 1 stop bit, oversampling by 16. */
    USART2->BRR = (CPU_HZ + CONSOLE_BAUD / 2U) / CONSOLE_BAUD;
    USART2->CR2 = 0;
    USART2->CR3 = 0;
    USART2->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    uart_ready = true;
}

void board_uart_putc(char c) {
    while (!(USART2->SR & USART_SR_TXE)) {
        /* Poll until the transmit register can accept a byte. */
    }
    USART2->DR = (uint8_t)c;
}

int board_uart_has_data(void) {
    return (USART2->SR & USART_SR_RXNE) != 0;
}

int board_uart_getc(void) {
    for (;;) {
        uint32_t status = USART2->SR;
        if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
            /* Reading SR then DR clears receive errors, including overrun. */
            uint32_t data = USART2->DR;
            if ((status & USART_SR_RXNE) && !(status & (USART_SR_NE | USART_SR_FE | USART_SR_PE))) {
                return (int)(data & 0xffU);
            }
        }
    }
}

void board_panic(const char *msg) {
    __disable_irq();
    if (uart_ready) {
        while (*msg) {
            board_uart_putc(*msg++);
        }
        while (!(USART2->SR & USART_SR_TC)) {
            /* Drain panic output before the permanent halt. */
        }
    }
    /* Fatal halt; normal execution cannot resume. */
    for (;;) {
        __WFI();
    }
}
