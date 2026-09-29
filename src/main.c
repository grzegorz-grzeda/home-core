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
#include "homecore/arch/arch.h"
#include "homecore/soc/soc.h"
#include "homecore/board/board.h"
#include "homecore/devicetree.h"
#include "homecore/vfs/vfs.h"
#include "homecore/kernel/kernel.h"
#include "homecore/shell/shell.h"
#include "homecore/version.h"
/*---------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
int main(void) {
    arch_init();
    soc_init();
    board_init();
    dt_init();
    k_init();
    shell_init();

    printf("\nHomeCore OS\n");
    printf("Version: %s\n", HOMECORE_VERSION_STRING);

    shell_run();

    return 0;
}
/*---------------------------------------------------------------------------*/
