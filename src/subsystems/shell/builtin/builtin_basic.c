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
#include "builtin_basic.h"
#include "g2basic.h"
#include "homecore/kernel/kernel.h"
#include "homecore/shell/shell.h"
#include <stdio.h>
#include <math.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define MAX_LINE_LENGTH 256
/*---------------------------------------------------------------------------*/
static char line[MAX_LINE_LENGTH] = {0};
/*---------------------------------------------------------------------------*/
static void basic_output(const char *text) {
    fputs(text, stdout);
}
/*---------------------------------------------------------------------------*/
/* Print nine significant digits without libc floating-point formatting.
 * Use scientific notation for very small or large values. */
static void basic_output_number(double value) {
    if (isnan(value)) {
        basic_output("nan");
        return;
    }
    if (signbit(value)) {
        putchar('-');
        value = -value;
    }
    if (isinf(value)) {
        basic_output("inf");
        return;
    }
    if (value == 0) {
        putchar('0');
        return;
    }

    int exponent = 0;
    while (value >= 10) {
        value /= 10;
        exponent++;
    }
    while (value < 1) {
        value *= 10;
        exponent--;
    }

    uint32_t rounded = (uint32_t)(value * 100000000 + 0.5);
    if (rounded == 1000000000) {
        rounded = 100000000;
        exponent++;
    }
    char digits[10];
    snprintf(digits, sizeof(digits), "%lu", (unsigned long)rounded);
    int last = 8;
    while (last > 0 && digits[last] == '0') {
        last--;
    }

    int scientific = exponent < -4 || exponent >= 9;
    int point = scientific ? 0 : exponent;
    if (point < 0) {
        basic_output("0.");
        for (int i = -1; i > point; i--) {
            putchar('0');
        }
    }
    for (int i = 0; i <= last || i <= point; i++) {
        putchar(i <= last ? digits[i] : '0');
        if (i == point && i < last) {
            putchar('.');
        }
    }
    if (scientific) {
        printf("e%+d", exponent);
    }
}
/*---------------------------------------------------------------------------*/
static double basic_millis(double args[], int count) {
    (void)args;
    (void)count;
    return (double)k_uptime_ms();
}

int shell_builtin_basic(shell_context_t *context, int argc, char **argv) {
    (void)context;
    (void)argc;
    (void)argv;
    g2basic_init(basic_output);
    g2basic_register_function("millis", 0, basic_millis);
    g2basic_set_number_output(basic_output_number);

    printf("G2BASIC Interpreter with line numbers. Ctrl-C/Ctrl-D/Ctrl-Z to "
           "exit.\n\n");

    while (1) {
        printf("> ");
        int len = shell_read_line(line, sizeof(line));
        if (len < 0) {
            break;
        }

        if (len == 0) {
            continue;
        }

        double val;
        const char *error = NULL;
        g2basic_parse(line, &val, &error);
        if (error) {
            printf("Error: %s\n", error);
        }
    }
    return 0;
}
/*---------------------------------------------------------------------------*/
