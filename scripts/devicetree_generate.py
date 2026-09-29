#!/usr/bin/env python3
"""Generate HomeCore's compile-time device description.

Starts from the board description, follows its `soc:` key to the SoC
description and the SoC's `arch:` key to the architecture, merges board and
overlays over the SoC, then validates every enabled device against its binding
(src/drivers/**/<compatible>.yaml). Writes devicetree.h (constants),
devicetree.c (driver instances and dt_init), the linker MEMORY block, and
devicetree.cmake (the selected SoC and architecture, linker fragments, and the
driver sources to compile). Nothing is parsed at run time.
"""
import argparse
import copy
import re
import sys
from pathlib import Path

import yaml

TYPES = ("int", "size", "string", "bool", "clock", "devpath")
DEVICE_KEYS = {"compatible", "status"}
# Keys each layer may set. The board selects the SoC and the SoC the
# architecture; overlays may change properties but not the selection.
LAYER_KEYS = {
    "soc": {"arch", "memory", "devices"},
    "board": {"soc", "name", "clock_setup", "memory", "clocks", "devices", "chosen", "mounts"},
    "overlay": {"clock_setup", "memory", "clocks", "devices", "chosen", "mounts"},
}
SELECTION = re.compile(r"^[a-z0-9_-]+(/[a-z0-9_-]+)*$")  # no "." so no ".."
MEMORY_REGIONS = {"FLASH": "rx", "RAM": "rwx"}
C_IDENTIFIER = re.compile(r"^[a-z][a-z0-9_]*$")
# Filesystem types a mount may name: header and function that mount one.
FILESYSTEMS = {"fat": ("homecore/fs/fat.h", "fs_fat_mount")}
MOUNT_KEYS = {"device", "fs", "format"}
MOUNT_PATH = re.compile(r"^/[a-z0-9_]+$")


class DescriptionError(Exception):
    pass


def load(path):
    try:
        data = yaml.safe_load(Path(path).read_text(encoding="utf-8")) or {}
    except (OSError, yaml.YAMLError) as error:
        raise DescriptionError(f"{path}: {error}") from error
    if not isinstance(data, dict):
        raise DescriptionError(f"{path}: expected a mapping at the top level")
    return data


def merge(base, overlay, where):
    """Deep-merge overlay into base; later layers win, mappings merge per key."""
    result = copy.deepcopy(base)
    for key, value in overlay.items():
        if isinstance(value, dict) and isinstance(result.get(key), dict):
            result[key] = merge(result[key], value, f"{where}.{key}")
        else:
            result[key] = copy.deepcopy(value)
    return result


def parse_size(value, where):
    if isinstance(value, bool):
        raise DescriptionError(f"{where}: invalid size {value!r}")
    if isinstance(value, int):
        return value
    match = re.fullmatch(r"(\d+)\s*([KM]?)", str(value))
    if not match:
        raise DescriptionError(f"{where}: invalid size {value!r}")
    return int(match.group(1)) * {"": 1, "K": 1024, "M": 1024 * 1024}[match.group(2)]


def load_bindings(bindings_dir):
    bindings = {}
    for path in sorted(Path(bindings_dir).rglob("*.yaml")):
        binding = load(path)
        compatible = binding.get("compatible")
        if not compatible or path.stem != compatible:
            raise DescriptionError(f"{path}: file name must be '<compatible>.yaml'")
        driver = binding.get("driver", "")
        if not C_IDENTIFIER.match(driver):
            raise DescriptionError(f"{path}: 'driver' must be a C identifier prefix")
        for name, spec in binding.get("properties", {}).items():
            if spec.get("type") not in TYPES:
                raise DescriptionError(f"{path}: property {name} has unknown type")
        binding["directory"] = path.parent
        bindings[compatible] = binding
    return bindings


def check_value(value, spec, where, clocks):
    kind = spec["type"]
    if kind == "size":
        value = parse_size(value, where)
        if value < spec.get("minimum", 0):
            raise DescriptionError(f"{where}: must be at least {spec.get('minimum', 0)}")
        if value % spec.get("multiple", 1):
            raise DescriptionError(f"{where}: must be a multiple of {spec['multiple']}")
        return value
    if kind == "int":
        if not isinstance(value, int) or isinstance(value, bool):
            raise DescriptionError(f"{where}: expected an integer")
        if value < spec.get("minimum", 0):
            raise DescriptionError(f"{where}: must be at least {spec.get('minimum', 0)}")
        return value
    if kind == "bool":
        if not isinstance(value, bool):
            raise DescriptionError(f"{where}: expected true or false")
        return value
    if kind == "clock":
        if value not in clocks:
            raise DescriptionError(f"{where}: clock {value!r} is not in the board's clocks")
        return clocks[value]
    if not isinstance(value, str) or not value:
        raise DescriptionError(f"{where}: expected a non-empty string")
    if kind == "devpath":
        if not re.fullmatch(r"[a-z0-9_]+", value):
            raise DescriptionError(f"{where}: device names use a-z, 0-9, and _")
        return "/dev/" + value
    return value


def check_layer(data, layer, path):
    unknown = set(data) - LAYER_KEYS[layer]
    if unknown:
        raise DescriptionError(f"{path}: keys not allowed in a {layer} description: "
                               f"{', '.join(sorted(unknown))}")


def select(board_path, soc_root, arch_root):
    """Resolve board -> SoC -> architecture. Returns (board, soc_name, soc_path, soc, arch)."""
    board = load(board_path)
    check_layer(board, "board", board_path)
    soc_name = board.get("soc")
    if not isinstance(soc_name, str) or not SELECTION.match(soc_name):
        raise DescriptionError(f"{board_path}: 'soc' must name a SoC directory, such as st/stm32f100")
    soc_path = Path(soc_root) / soc_name / "soc.yaml"
    if not soc_path.is_file():
        raise DescriptionError(f"{board_path}: unknown SoC {soc_name!r}; missing {soc_path}")
    soc = load(soc_path)
    check_layer(soc, "soc", soc_path)
    arch = soc.get("arch")
    if not isinstance(arch, str) or not SELECTION.match(arch):
        raise DescriptionError(f"{soc_path}: 'arch' must name an architecture, such as arm/cortex-m3")
    if not (Path(arch_root) / arch / "arch.cmake").is_file():
        raise DescriptionError(f"{soc_path}: unknown architecture {arch!r}; "
                               f"missing {Path(arch_root) / arch / 'arch.cmake'}")
    return board, soc_name, soc_path, soc, arch


def resolve(description, bindings):

    memory = {}
    for region, attributes in MEMORY_REGIONS.items():
        spec = description.get("memory", {}).get(region.lower())
        if not isinstance(spec, dict) or "base" not in spec or "size" not in spec:
            raise DescriptionError(f"memory.{region.lower()}: 'base' and 'size' are required")
        if not isinstance(spec["base"], int) or isinstance(spec["base"], bool) or spec["base"] < 0:
            raise DescriptionError(f"memory.{region.lower()}.base: expected an address")
        size = parse_size(spec["size"], f"memory.{region.lower()}.size")
        if size <= 0:
            raise DescriptionError(f"memory.{region.lower()}.size: must be positive")
        memory[region] = (attributes, spec["base"], size)

    clocks = description.get("clocks", {})
    if not isinstance(clocks.get("cpu"), int) or clocks["cpu"] <= 0:
        raise DescriptionError("clocks.cpu: a positive CPU clock in Hz is required")
    clock_setup = description.get("clock_setup", True)
    if not isinstance(clock_setup, bool):
        raise DescriptionError("clock_setup: expected true or false")

    devices = []
    seen = {}
    for name, node in description.get("devices", {}).items():
        where = f"devices.{name}"
        if not C_IDENTIFIER.match(name):
            raise DescriptionError(f"{where}: node names must be C identifiers")
        status = node.get("status", "okay")
        if status not in ("okay", "disabled"):
            raise DescriptionError(f"{where}.status: must be okay or disabled")
        binding = bindings.get(node.get("compatible"))
        if binding is None:
            raise DescriptionError(f"{where}: no binding for {node.get('compatible')!r}")
        if status == "disabled":
            continue
        specs = binding.get("properties", {})
        unknown = set(node) - DEVICE_KEYS - set(specs)
        if unknown:
            raise DescriptionError(f"{where}: unknown properties {', '.join(sorted(unknown))}")
        values = {}
        for prop, spec in specs.items():
            if prop in node:
                value = check_value(node[prop], spec, f"{where}.{prop}", clocks)
            elif spec.get("type") == "devpath":
                value = "/dev/" + name
            elif "default" in spec:
                value = check_value(spec["default"], spec, f"{where}.{prop}", clocks)
            elif spec.get("required"):
                raise DescriptionError(f"{where}: missing required property {prop}")
            else:
                continue
            values[prop] = value
            if spec.get("unique") or spec.get("type") == "devpath":
                key = (prop, value)
                if key in seen:
                    raise DescriptionError(f"{where}.{prop}: {value!r} also used by {seen[key]}")
                seen[key] = name
        devices.append({"name": name, "binding": binding, "values": values})

    console = description.get("chosen", {}).get("console")
    by_name = {device["name"]: device for device in devices}
    if console not in by_name:
        raise DescriptionError(f"chosen.console: {console!r} is not an enabled device")
    if not by_name[console]["binding"].get("console"):
        raise DescriptionError(f"chosen.console: {console!r} cannot act as a console")
    return memory, clocks, clock_setup, devices, by_name[console], resolve_mounts(description, by_name)


def resolve_mounts(description, by_name):
    mounts = []
    specs = description.get("mounts", {})
    if not isinstance(specs, dict):
        raise DescriptionError("mounts: expected a mapping of mount-point paths")
    used = set()
    for path, spec in specs.items():
        where = f"mounts.{path}"
        if not isinstance(path, str) or not MOUNT_PATH.match(path) or path == "/dev":
            raise DescriptionError(f"{where}: mount points are top-level paths such as /ram")
        if not isinstance(spec, dict) or set(spec) - MOUNT_KEYS:
            raise DescriptionError(f"{where}: allowed keys are {', '.join(sorted(MOUNT_KEYS))}")
        device = by_name.get(spec.get("device"))
        if device is None or not device["binding"].get("block"):
            raise DescriptionError(f"{where}.device: {spec.get('device')!r} "
                                   "is not an enabled block device")
        if device["name"] in used:
            raise DescriptionError(f"{where}.device: {device['name']!r} is already mounted")
        used.add(device["name"])
        if spec.get("fs") not in FILESYSTEMS:
            raise DescriptionError(f"{where}.fs: expected one of {', '.join(sorted(FILESYSTEMS))}")
        format_empty = spec.get("format", False)
        if not isinstance(format_empty, bool):
            raise DescriptionError(f"{where}.format: expected true or false")
        mounts.append({"path": path, "device": device, "fs": spec["fs"], "format": format_empty})
    return mounts


def c_literal(value, hexadecimal=False):
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, int):
        return f"0x{value:X}U" if hexadecimal else f"{value}U"
    return '"' + value.replace("\\", "\\\\").replace('"', '\\"') + '"'


def write_if_changed(path, text):
    path = Path(path)
    if not path.exists() or path.read_text(encoding="utf-8") != text:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")


def generate(memory, clocks, clock_setup, devices, console, mounts, selection, args):
    banner = "/* Generated by scripts/devicetree_generate.py. Do not edit. */\n"
    header = [banner, "#ifndef HOMECORE_DEVICETREE_H", "#define HOMECORE_DEVICETREE_H", ""]
    header.append(f"#define DT_BOARD_NAME {c_literal(selection['name'])}")
    header.append(f"#define DT_CPU_CLOCK_HZ {clocks['cpu']}U")
    header.append("/* 1: board_init() configures the clocks; 0: an emulator provides them. */")
    header.append(f"#define DT_CLOCK_SETUP {1 if clock_setup else 0}")
    console_path = console["values"].get("devname")
    header.append(f"#define DT_CHOSEN_CONSOLE_PATH {c_literal(console_path)}")
    header.append(f"#define DT_DEVICE_COUNT {len(devices)}U")
    header.append(f"#define DT_MOUNT_COUNT {len(mounts)}U")
    header += ["", "/* Initialize every enabled device, in description order. */",
               "void dt_init(void);",
               "/* Run the handler of the device that owns peripheral interrupt irq;",
               " * panic for an interrupt no enabled device claims. Called by",
               " * arch_irq_entry() in interrupt context. */",
               "void dt_irq_dispatch(unsigned irq);",
               "/* Mount every filesystem in the mounts section, in order. Reports a",
               " * failed mount on stdout and continues. Call after k_init(). */",
               "void dt_mount_all(void);", "", "#endif /* HOMECORE_DEVICETREE_H */", ""]

    source = [banner, '#include "homecore/devicetree.h"', '#include "homecore/board/board.h"',
              '#include "homecore/drivers/console.h"', "#include <stdint.h>"]
    filesystems = sorted({mount["fs"] for mount in mounts})
    for fs in filesystems:
        source.append(f'#include "{FILESYSTEMS[fs][0]}"')
    if mounts:
        source += ["#include <errno.h>", "#include <stdio.h>", "#include <string.h>"]
    for driver in sorted({device["binding"]["driver"] for device in devices}):
        source.append(f'#include "{driver}.h"')
    source.append("")
    for device in devices:
        driver = device["binding"]["driver"]
        fields = []
        for prop, spec in device["binding"].get("properties", {}).items():
            if "field" in spec and prop in device["values"]:
                literal = c_literal(device["values"][prop], hexadecimal=prop == "reg")
                fields.append(f"    .{spec['field']} = {literal},")
            if "buffer" in spec and prop in device["values"]:
                buffer = f"dt_{device['name']}_{spec['buffer']}"
                source.append(f"static uint8_t {buffer}[{device['values'][prop]}] "
                              "__attribute__((aligned(4)));")
                fields.append(f"    .{spec['buffer']} = {buffer},")
        source.append(f"static const {driver}_config_t dt_{device['name']}_config = {{")
        source += fields
        source.append("};")
        source.append(f"static {driver}_t dt_{device['name']} = "
                      f"{{.config = &dt_{device['name']}_config}};")
    source += ["", "const console_t dt_console = {",
               f"    .ops = &{console['binding']['driver']}_console_ops,",
               f"    .device = &dt_{console['name']},", "};", "", "void dt_init(void) {"]
    for device in devices:
        source.append(f"    {device['binding']['driver']}_init(&dt_{device['name']});")
    source += ["}", "", "void dt_irq_dispatch(unsigned irq) {", "    switch (irq) {"]
    for device in devices:
        if device["binding"].get("isr") and "irq" in device["values"]:
            source += [f"    case {device['values']['irq']}U:",
                       f"        {device['binding']['driver']}_isr(&dt_{device['name']});",
                       "        return;"]
    source += ["    default:", "        break;", "    }",
               '    board_panic("Unexpected interrupt");', "}", ""]
    for mount in mounts:
        device = mount["device"]
        source += [f"static const block_device_t dt_{device['name']}_block = {{",
                   f"    .ops = &{device['binding']['driver']}_block_ops,",
                   f"    .device = &dt_{device['name']},", "};", ""]
    source.append("void dt_mount_all(void) {")
    for mount in mounts:
        call = (f"{FILESYSTEMS[mount['fs']][1]}(&dt_{mount['device']['name']}_block, "
                f"{c_literal(mount['path'])}, {c_literal(mount['format'])})")
        source += [f"    if ({call} < 0) {{",
                   f'        printf("mount {mount["path"]}: %s\\n", strerror(errno));',
                   "    }"]
    source += ["}", ""]

    memory_lines = [f"    {region} ({attributes}) : ORIGIN = 0x{base:08X}, LENGTH = {size}"
                    for region, (attributes, base, size) in memory.items()]

    sources = sorted({str(device["binding"]["directory"] / src)
                      for device in devices for src in device["binding"].get("sources", [])})
    directories = sorted({str(device["binding"]["directory"]) for device in devices})
    empty_fragment = Path(args.memory).parent / "empty.ld"
    write_if_changed(empty_fragment, "/* No additional sections. */\n")

    def fragment(directory, name):
        path = Path(directory) / name
        return path if path.is_file() else empty_fragment

    soc_dir = selection["soc_path"].parent
    cmake = ["# Generated by scripts/devicetree_generate.py. Do not edit.",
             f'set(HOMECORE_SOC "{selection["soc"]}")',
             f'set(HOMECORE_SOC_DIR "{soc_dir.resolve()}")',
             f'set(HOMECORE_ARCH "{selection["arch"]}")',
             f'set(HOMECORE_SOC_LINKER_SCRIPT "{fragment(soc_dir, "soc.ld").resolve()}")',
             f'set(HOMECORE_BOARD_LINKER_SCRIPT '
             f'"{fragment(Path(args.board).parent, "board.ld").resolve()}")',
             "set(HOMECORE_DT_INPUT_FILES",
             *[f"    {Path(path).resolve()}" for path in selection["inputs"]], ")",
             "set(HOMECORE_DT_SOURCES", *[f"    {s}" for s in sources], ")",
             "set(HOMECORE_DT_INCLUDE_DIRS", *[f"    {d}" for d in directories], ")",
             f'set(HOMECORE_DT_FILESYSTEMS "{";".join(filesystems)}")', ""]

    write_if_changed(args.header, "\n".join(header))
    write_if_changed(args.source, "\n".join(source))
    write_if_changed(args.memory, "\n".join(memory_lines) + "\n")
    write_if_changed(args.cmake, "\n".join(cmake))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--board", required=True, help="board description (board.yaml)")
    parser.add_argument("--soc-root", required=True, help="directory holding <soc>/soc.yaml")
    parser.add_argument("--arch-root", required=True, help="directory holding <arch>/arch.cmake")
    parser.add_argument("--overlay", action="append", default=[], help="overlay, applied in order")
    parser.add_argument("--bindings", required=True, help="directory searched for bindings")
    parser.add_argument("--header", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--memory", required=True)
    parser.add_argument("--cmake", required=True)
    args = parser.parse_args()
    try:
        board, soc_name, soc_path, soc, arch = select(args.board, args.soc_root, args.arch_root)
        description = merge(soc, board, "board")
        for path in args.overlay:
            overlay = load(path)
            check_layer(overlay, "overlay", path)
            description = merge(description, overlay, Path(path).name)
        for key in ("soc", "arch", "name"):
            description.pop(key, None)
        selection = {"name": board.get("name", Path(args.board).parent.name),
                     "soc": soc_name, "soc_path": soc_path, "arch": arch,
                     "inputs": [soc_path, args.board, *args.overlay]}
        if not isinstance(selection["name"], str) or not selection["name"]:
            raise DescriptionError(f"{args.board}: 'name' must be a non-empty string")
        bindings = load_bindings(args.bindings)
        generate(*resolve(description, bindings), selection, args)
    except DescriptionError as error:
        print(f"devicetree: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
