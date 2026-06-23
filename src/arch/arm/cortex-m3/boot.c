/**
 * MIT License
 *
 * Copyright (c) 2026 Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include "homecore/board/board.h"
/*---------------------------------------------------------------------------*/
extern uint32_t _estack;
/*---------------------------------------------------------------------------*/
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;
/*---------------------------------------------------------------------------*/
extern int main(void);
/*---------------------------------------------------------------------------*/
void Reset_Handler(void);
/*---------------------------------------------------------------------------*/
void NMI_Handler(void);
/*---------------------------------------------------------------------------*/
void HardFault_Handler(void);
/*---------------------------------------------------------------------------*/
void MemManage_Handler(void);
/*---------------------------------------------------------------------------*/
void BusFault_Handler(void);
/*---------------------------------------------------------------------------*/
void UsageFault_Handler(void);
/*---------------------------------------------------------------------------*/
void SVC_Handler(void);
/*---------------------------------------------------------------------------*/
void DebugMon_Handler(void);
/*---------------------------------------------------------------------------*/
void PendSV_Handler(void);
/*---------------------------------------------------------------------------*/
void SysTick_Handler(void);
/*---------------------------------------------------------------------------*/
__attribute__((section(".isr_vector.arch"))) const void *vector_table[] = {
    &_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,
};
/*---------------------------------------------------------------------------*/
void Reset_Handler(void) {
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;

    while (dst < &_edata) {
        *dst++ = *src++;
    }

    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    int result = main();

    if (result != 0) {
        board_panic("main() returned non-zero value");
    }

    while (1) {
    }
}
/*---------------------------------------------------------------------------*/
void NMI_Handler(void) {
    board_panic("NMI_Handler");
}
/*---------------------------------------------------------------------------*/
void HardFault_Handler(void) {
    board_panic("HardFault_Handler");
}
/*---------------------------------------------------------------------------*/
void MemManage_Handler(void) {
    board_panic("MemManage_Handler");
}
/*---------------------------------------------------------------------------*/
void BusFault_Handler(void) {
    board_panic("BusFault_Handler");
}
/*---------------------------------------------------------------------------*/
void UsageFault_Handler(void) {
    board_panic("UsageFault_Handler");
}
/*---------------------------------------------------------------------------*/
void SVC_Handler(void) {
    board_panic("SVC_Handler");
}
/*---------------------------------------------------------------------------*/
void DebugMon_Handler(void) {
    board_panic("DebugMon_Handler");
}
/*---------------------------------------------------------------------------*/
void PendSV_Handler(void) {
    board_panic("PendSV_Handler");
}
/*---------------------------------------------------------------------------*/
void SysTick_Handler(void) {
    board_panic("SysTick_Handler");
}
/*---------------------------------------------------------------------------*/
