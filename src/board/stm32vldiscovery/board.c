// SPDX-License-Identifier: MIT
#include "homecore/board/board.h"
#include "homecore/autoconf.h"
#include "soc_cmsis.h"
#include <stdbool.h>

/* 24 MHz is the STM32F100 maximum. It is produced by the PLL below, and it is
 * also the fixed CPU clock of QEMU's stm32vldiscovery machine. */
#define CPU_HZ       24000000U
#define CONSOLE_BAUD 115200U
/* Iteration budget: startup has no timer yet; this is not a millisecond timeout. */
#define CLOCK_WAIT_LIMIT 1000000U
/* GPIOA->CRH fields for PA9 (bits 4-7) and PA10 (bits 8-11). */
#define PA9_CRH_MASK   (0xFU << GPIO_CRH_MODE9_Pos)
#define PA10_CRH_MASK  (0xFU << GPIO_CRH_MODE10_Pos)
#define PA9_TX_AF_PP   (0xAU << GPIO_CRH_MODE9_Pos)  /* CNF=10 AF push-pull, MODE=10 2 MHz */
#define PA10_RX_PULLED (0x8U << GPIO_CRH_MODE10_Pos) /* CNF=10 input pull, MODE=00 */
#define PA10_PULL_UP   (1U << 10)
/* RCC->CFGR fields set by the PLL configuration. */
#define CFGR_PLL_AND_BUS_FIELDS                                                                    \
    (RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL | RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2)
static bool uart_ready;

#if defined(CONFIG_HOMECORE_BOARD_CLOCK_SETUP)
static bool wait_clock(volatile const uint32_t *reg, uint32_t mask, uint32_t value) {
    for (uint32_t attempt = 0; attempt < CLOCK_WAIT_LIMIT; ++attempt) {
        if ((*reg & mask) == value) {
            return true;
        }
    }
    return false;
}

/* SYSCLK = PLL = HSI/2 * 6 = 24 MHz with undivided AHB, APB1, and APB2 clocks.
 * The value line has no flash wait states or prefetch control, so the flash
 * interface needs no setup. Failures halt before the console is ready. */
static void configure_clock(void) {
    RCC->CR |= RCC_CR_HSION;
    if (!wait_clock(&RCC->CR, RCC_CR_HSIRDY, RCC_CR_HSIRDY)) {
        board_panic("HSI did not become ready");
        return;
    }
    /* Run from HSI while the PLL is reconfigured. */
    RCC->CFGR &= ~RCC_CFGR_SW;
    if (!wait_clock(&RCC->CFGR, RCC_CFGR_SWS, 0U)) {
        board_panic("Cannot switch system clock to HSI");
        return;
    }
    RCC->CR &= ~RCC_CR_PLLON;
    if (!wait_clock(&RCC->CR, RCC_CR_PLLRDY, 0U)) {
        board_panic("PLL did not stop");
        return;
    }
    /* PLLSRC = 0 selects HSI/2; zero prescalers leave AHB, APB1, and APB2 undivided. */
    RCC->CFGR = (RCC->CFGR & ~CFGR_PLL_AND_BUS_FIELDS) | RCC_CFGR_PLLMULL6;
    RCC->CR |= RCC_CR_PLLON;
    if (!wait_clock(&RCC->CR, RCC_CR_PLLRDY, RCC_CR_PLLRDY)) {
        board_panic("PLL did not lock");
        return;
    }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    if (!wait_clock(&RCC->CFGR, RCC_CFGR_SWS, RCC_CFGR_SWS_PLL)) {
        board_panic("Cannot switch system clock to PLL");
        return;
    }
}
#endif

uint32_t board_cpu_clock_hz(void) {
    return CPU_HZ;
}

void board_init(void) {
#if defined(CONFIG_HOMECORE_BOARD_CLOCK_SETUP)
    configure_clock();
#else
    /* Emulator build: QEMU does not model RCC and runs the CPU at CPU_HZ. */
#endif

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;

    /* PA9 = USART1_TX (alternate push-pull), PA10 = USART1_RX (input, pull-up). */
    GPIOA->CRH = (GPIOA->CRH & ~(PA9_CRH_MASK | PA10_CRH_MASK)) | PA9_TX_AF_PP | PA10_RX_PULLED;
    GPIOA->ODR |= PA10_PULL_UP;

    /* 115200 baud, 8 data bits, no parity, 1 stop bit; USART1 runs on APB2. */
    USART1->BRR = (CPU_HZ + CONSOLE_BAUD / 2U) / CONSOLE_BAUD;
    USART1->CR2 = 0;
    USART1->CR3 = 0;
    USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    uart_ready = true;
}

void board_uart_putc(char c) {
    while (!(USART1->SR & USART_SR_TXE)) {
        /* Poll until the transmit register can accept a byte. */
    }
    USART1->DR = (uint8_t)c;
}

int board_uart_has_data(void) {
    return (USART1->SR & USART_SR_RXNE) != 0;
}

int board_uart_getc(void) {
    for (;;) {
        uint32_t status = USART1->SR;
        if (status & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)) {
            /* Reading SR then DR clears receive errors, including overrun. */
            uint32_t data = USART1->DR;
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
        while (!(USART1->SR & USART_SR_TC)) {
            /* Drain panic output before the permanent halt. */
        }
    }
    /* Fatal halt; normal execution cannot resume. */
    for (;;) {
        __WFI();
    }
}
