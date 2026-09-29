/* SPDX-License-Identifier: MIT */
/* Register-level contract of the STM32 GPIO drivers against fake registers.
 * Built once per family: -DTEST_STM32F1 selects the F1 driver. */
#include "soc_cmsis.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#ifdef TEST_STM32F1
#include "../src/drivers/gpio/stm32f1_gpio.c"
#else
#include "../src/drivers/gpio/stm32f4_gpio.c"
#endif

RCC_TypeDef test_rcc;
static GPIO_TypeDef port_regs;

/* BSRR is write-only on hardware; apply a write to ODR and IDR like the pins
 * would, then clear it. */
static void apply_bsrr(void) {
    uint32_t bsrr = port_regs.BSRR;
    port_regs.ODR = (port_regs.ODR | (bsrr & 0xFFFFU)) & ~(bsrr >> 16);
    port_regs.IDR = port_regs.ODR;
    port_regs.BSRR = 0;
}

static void check_common(const gpio_ops_t *ops, void *port) {
    bool high = true;
    assert(ops->set(port, 5, true) == 0 && port_regs.BSRR == 1U << 5);
    apply_bsrr();
    assert(ops->get(port, 5, &high) == 0 && high);
    assert(ops->set(port, 5, false) == 0 && port_regs.BSRR == 1U << 21);
    apply_bsrr();
    assert(ops->get(port, 5, &high) == 0 && !high);
    errno = 0;
    assert(ops->set(port, 16, true) == -1 && errno == EINVAL && port_regs.BSRR == 0);
    assert(ops->get(port, 16, &high) == -1 && errno == EINVAL);
    assert(ops->configure(port, 16, GPIO_OUTPUT) == -1 && errno == EINVAL);
    assert(ops->configure(port, 0, (gpio_mode_t)99) == -1 && errno == EINVAL);
}

#ifdef TEST_STM32F1
int main(void) {
    static const stm32f1_gpio_config_t config = {.base = (uintptr_t)&port_regs, .clock_bit = 4};
    stm32f1_gpio_t dev = {.config = &config};
    port_regs.CRL = port_regs.CRH = 0x44444444U; /* Reset: floating inputs. */
    stm32f1_gpio_init(&dev);
    assert(test_rcc.APB2ENR == 1U << 4);
    const gpio_ops_t *ops = &stm32f1_gpio_gpio_ops;
    assert(ops->configure(&dev, 9, GPIO_OUTPUT) == 0);
    assert(port_regs.CRH == 0x44444424U && port_regs.CRL == 0x44444444U);
    assert(ops->configure(&dev, 1, GPIO_INPUT_PULL_UP) == 0);
    assert(port_regs.CRL == 0x44444484U && port_regs.BSRR == 1U << 1);
    apply_bsrr();
    assert(ops->configure(&dev, 1, GPIO_INPUT_PULL_DOWN) == 0 && port_regs.BSRR == 1U << 17);
    apply_bsrr();
    assert(ops->configure(&dev, 1, GPIO_INPUT) == 0 && port_regs.CRL == 0x44444444U);
    assert(ops->configure(&dev, 15, GPIO_OUTPUT) == 0 && port_regs.CRH == 0x24444424U);
    check_common(ops, &dev);
    puts("PASS: STM32F1 GPIO clock, CRL/CRH modes, pulls, BSRR output, IDR input, pin checks");
    return 0;
}
#else
int main(void) {
    static const stm32f4_gpio_config_t config = {.base = (uintptr_t)&port_regs, .clock_bit = 3};
    stm32f4_gpio_t dev = {.config = &config};
    port_regs.OTYPER = 1U << 12; /* Open drain before configuration. */
    port_regs.OSPEEDR = 3U << 24;
    port_regs.MODER = 0xA8000000U; /* Unrelated pins keep their modes. */
    stm32f4_gpio_init(&dev);
    assert(test_rcc.AHB1ENR == 1U << 3);
    const gpio_ops_t *ops = &stm32f4_gpio_gpio_ops;
    assert(ops->configure(&dev, 12, GPIO_OUTPUT) == 0);
    assert(port_regs.MODER == 0xA9000000U && port_regs.OTYPER == 0 && port_regs.OSPEEDR == 0);
    assert(port_regs.PUPDR == 0);
    assert(ops->configure(&dev, 2, GPIO_INPUT_PULL_UP) == 0 && port_regs.PUPDR == 1U << 4);
    assert(ops->configure(&dev, 2, GPIO_INPUT_PULL_DOWN) == 0 && port_regs.PUPDR == 2U << 4);
    assert(ops->configure(&dev, 2, GPIO_INPUT) == 0 && port_regs.PUPDR == 0);
    assert(ops->configure(&dev, 12, GPIO_INPUT) == 0 && port_regs.MODER == 0xA8000000U);
    check_common(ops, &dev);
    puts(
        "PASS: STM32F4 GPIO clock, MODER/OTYPER/OSPEEDR/PUPDR, BSRR output, IDR input, pin checks");
    return 0;
}
#endif
