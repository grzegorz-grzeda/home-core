// SPDX-License-Identifier: MIT
#include "stm32f1_gpio.h"
#include "soc_cmsis.h"
#include <errno.h>

#define GPIO_PINS 16U
/* Four-bit CRL/CRH values (CNF[1:0] MODE[1:0]). */
#define CONFIG_INPUT_FLOATING 0x4U
#define CONFIG_INPUT_PULL     0x8U /* ODR selects pull-up (1) or pull-down (0) */
#define CONFIG_OUTPUT_2MHZ    0x2U /* General-purpose push-pull output */

static GPIO_TypeDef *regs(const void *port) {
    return (GPIO_TypeDef *)((const stm32f1_gpio_t *)port)->config->base;
}

static int configure(void *port, unsigned pin, gpio_mode_t mode) {
    if (pin >= GPIO_PINS) {
        errno = EINVAL;
        return -1;
    }
    GPIO_TypeDef *gpio = regs(port);
    uint32_t config;
    switch (mode) {
    case GPIO_INPUT:
        config = CONFIG_INPUT_FLOATING;
        break;
    case GPIO_INPUT_PULL_UP:
        config = CONFIG_INPUT_PULL;
        gpio->BSRR = 1UL << pin;
        break;
    case GPIO_INPUT_PULL_DOWN:
        config = CONFIG_INPUT_PULL;
        gpio->BSRR = 1UL << (pin + 16U);
        break;
    case GPIO_OUTPUT:
        config = CONFIG_OUTPUT_2MHZ;
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    /* Pins 0-7 are in CRL and 8-15 in CRH, four bits each. */
    volatile uint32_t *reg = pin < 8U ? &gpio->CRL : &gpio->CRH;
    uint32_t shift = 4U * (pin % 8U);
    *reg = (*reg & ~(0xFUL << shift)) | (config << shift);
    return 0;
}

/* BSRR sets or resets one pin atomically, without read-modify-write. */
static int set(void *port, unsigned pin, bool high) {
    if (pin >= GPIO_PINS) {
        errno = EINVAL;
        return -1;
    }
    regs(port)->BSRR = high ? (1UL << pin) : (1UL << (pin + 16U));
    return 0;
}

static int get(void *port, unsigned pin, bool *high) {
    if (pin >= GPIO_PINS) {
        errno = EINVAL;
        return -1;
    }
    *high = (regs(port)->IDR & (1UL << pin)) != 0U;
    return 0;
}

const gpio_ops_t stm32f1_gpio_gpio_ops = {
    .configure = configure,
    .set = set,
    .get = get,
};

void stm32f1_gpio_init(stm32f1_gpio_t *dev) {
    RCC->APB2ENR |= 1UL << dev->config->clock_bit;
    (void)RCC->APB2ENR; /* Complete the enable before the first access. */
}
