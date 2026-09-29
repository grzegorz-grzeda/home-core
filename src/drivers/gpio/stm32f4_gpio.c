// SPDX-License-Identifier: MIT
#include "stm32f4_gpio.h"
#include "soc_cmsis.h"
#include <errno.h>

#define GPIO_PINS 16U
/* Two-bit MODER and PUPDR field values. */
#define MODE_INPUT  0U
#define MODE_OUTPUT 1U
#define PULL_NONE   0U
#define PULL_UP     1U
#define PULL_DOWN   2U

static GPIO_TypeDef *regs(const void *port) {
    return (GPIO_TypeDef *)((const stm32f4_gpio_t *)port)->config->base;
}

static uint32_t field(uint32_t value, uint32_t current, unsigned pin) {
    uint32_t shift = 2U * pin;
    return (current & ~(3UL << shift)) | (value << shift);
}

static int configure(void *port, unsigned pin, gpio_mode_t mode) {
    if (pin >= GPIO_PINS) {
        errno = EINVAL;
        return -1;
    }
    GPIO_TypeDef *gpio = regs(port);
    uint32_t pull = PULL_NONE;
    uint32_t moder = MODE_INPUT;
    switch (mode) {
    case GPIO_INPUT:
        break;
    case GPIO_INPUT_PULL_UP:
        pull = PULL_UP;
        break;
    case GPIO_INPUT_PULL_DOWN:
        pull = PULL_DOWN;
        break;
    case GPIO_OUTPUT:
        moder = MODE_OUTPUT;
        /* Push-pull (OTYPER 0) at low speed (OSPEEDR 0). */
        gpio->OTYPER &= ~(1UL << pin);
        gpio->OSPEEDR = field(0U, gpio->OSPEEDR, pin);
        break;
    default:
        errno = EINVAL;
        return -1;
    }
    gpio->PUPDR = field(pull, gpio->PUPDR, pin);
    gpio->MODER = field(moder, gpio->MODER, pin);
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

const gpio_ops_t stm32f4_gpio_gpio_ops = {
    .configure = configure,
    .set = set,
    .get = get,
};

void stm32f4_gpio_init(stm32f4_gpio_t *dev) {
    RCC->AHB1ENR |= 1UL << dev->config->clock_bit;
    /* The read-back delays the first register access until the clock is
     * running (STM32F40x errata: 2 cycles after enabling a clock). */
    (void)RCC->AHB1ENR;
}
