// SPDX-License-Identifier: MIT
#include "homecore/board/board.h"
#include "homecore/autoconf.h"
#include "homecore/devicetree.h"
#include "homecore/drivers/console.h"
#include "soc_cmsis.h"
#include <stdbool.h>

/* 24 MHz is the STM32F100 maximum. It is produced by the PLL below, and it is
 * also the fixed CPU clock of QEMU's stm32vldiscovery machine. */
_Static_assert(DT_CPU_CLOCK_HZ == 24000000U, "board.yaml clocks must match the PLL setup");
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
    return DT_CPU_CLOCK_HZ;
}

void board_init(void) {
#if defined(CONFIG_HOMECORE_BOARD_CLOCK_SETUP)
    configure_clock();
#else
    /* Emulator build: QEMU does not model RCC and runs the CPU at DT_CPU_CLOCK_HZ. */
#endif

    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN | RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;
    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;
    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;

    /* Console USART1 (board.yaml): PA9 = TX (alternate push-pull), PA10 = RX
     * (input, pull-up). The serial driver programs the USART in dt_init(). */
    GPIOA->CRH = (GPIOA->CRH & ~(PA9_CRH_MASK | PA10_CRH_MASK)) | PA9_TX_AF_PP | PA10_RX_PULLED;
    GPIOA->ODR |= PA10_PULL_UP;
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
