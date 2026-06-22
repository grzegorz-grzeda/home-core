#!/usr/bin/env bash
set -e

qemu-system-arm \
-M lm3s6965evb \
-kernel build/qemu-lm3s6965evb/homecore \
-nographic