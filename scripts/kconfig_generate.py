#!/usr/bin/env python3

import argparse
import os
import sys

import kconfiglib


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--kconfig", required=True)
    parser.add_argument("--defconfig", required=True)
    parser.add_argument("--config-out", required=True)
    parser.add_argument("--header-out", required=True)
    args = parser.parse_args()

    os.environ["srctree"] = os.getcwd()

    kconf = kconfiglib.Kconfig(args.kconfig)

    if os.path.exists(args.defconfig):
        kconf.load_config(args.defconfig)
    else:
        print(f"error: defconfig not found: {args.defconfig}", file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(args.config_out), exist_ok=True)
    os.makedirs(os.path.dirname(args.header_out), exist_ok=True)

    kconf.write_config(args.config_out)
    kconf.write_autoconf(args.header_out)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())