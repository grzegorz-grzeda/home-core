#!/usr/bin/env bash
set -e

qemu-system-arm \
-M lm3s6965evb \
-kernel build/lm3s6965evb/homecore \
-nographic