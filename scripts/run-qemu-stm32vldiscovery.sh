#!/usr/bin/env bash
set -e

# QEMU maps its first serial port to USART1, the board console.
qemu-system-arm \
-M stm32vldiscovery \
-kernel build/stm32vldiscovery-qemu/homecore \
-nographic
