/* SPDX-License-Identifier: MIT */
/* Host-test stand-in for the LM3S device header: the UART registers used by
 * src/drivers/serial/stellaris_uart.c as plain memory. */
#ifndef HOMECORE_TEST_STELLARIS_SOC_CMSIS_H
#define HOMECORE_TEST_STELLARIS_SOC_CMSIS_H
#include <stdint.h>
typedef struct {
    volatile uint32_t DR;
    volatile uint32_t FR;
} UART0_Type;
#endif
