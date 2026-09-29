// SPDX-License-Identifier: MIT
#include "homecore/board/board.h"
#include "homecore/devicetree.h"
#include "homecore/drivers/console.h"
#include "soc_cmsis.h"
#include <stdbool.h>

/* board_init() selects the 16 MHz HSI with undivided buses; board.yaml must agree. */
_Static_assert(DT_CPU_CLOCK_HZ == 16000000U, "board.yaml clocks must match the HSI setup");
/* Iteration budget: startup has no timer yet; this is not a millisecond timeout. */
#define CLOCK_WAIT_LIMIT 1000000U

static bool wait_clock(volatile const uint32_t *reg, uint32_t mask, uint32_t value) {
    for (uint32_t attempt = 0; attempt < CLOCK_WAIT_LIMIT; ++attempt) {
        if ((*reg & mask) == value) {
            return true;
        }
    }
    return false;
}

uint32_t board_cpu_clock_hz(void) {
    return DT_CPU_CLOCK_HZ;
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

    /* Console USART2 (board.yaml): PA2 = TX, PA3 = RX, alternate function 7.
     * The serial driver programs the USART itself in dt_init(). */
    GPIOA->MODER = (GPIOA->MODER & ~((3U << 4) | (3U << 6))) | (2U << 4) | (2U << 6);
    GPIOA->OTYPER &= ~((1U << 2) | (1U << 3));
    GPIOA->OSPEEDR = (GPIOA->OSPEEDR & ~((3U << 4) | (3U << 6))) | (2U << 4) | (2U << 6);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~((3U << 4) | (3U << 6))) | (1U << 6);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~((15U << 8) | (15U << 12))) | (7U << 8) | (7U << 12);
}

void board_panic(const char *msg) {
    __disable_irq();
    if (console_ready()) {
        while (*msg) {
            board_uart_putc(*msg++);
        }
        console_flush(); /* Drain panic output before the permanent halt. */
    }
    /* Fatal halt; normal execution cannot resume. */
    for (;;) {
        __WFI();
    }
}
