#!/usr/bin/env python3
"""Generate HomeCore's compile-time device description.

Merges the SoC description, the board description, and optional overlays, then
validates every enabled device against its binding (src/drivers/**/<compatible>.yaml).
Writes devicetree.h (constants), devicetree.c (driver instances and dt_init),
the linker MEMORY block, and devicetree.cmake (driver sources to compile).
Nothing is parsed at run time.
"""
import argparse
import copy
import re
import sys
from pathlib import Path

import yaml

TYPES = ("int", "string", "bool", "clock", "devpath")
DEVICE_KEYS = {"compatible", "status"}
TOP_KEYS = {"memory", "clocks", "devices", "chosen"}
MEMORY_REGIONS = {"FLASH": "rx", "RAM": "rwx"}
C_IDENTIFIER = re.compile(r"^[a-z][a-z0-9_]*$")


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


def resolve(description, bindings):
    unknown = set(description) - TOP_KEYS
    if unknown:
        raise DescriptionError(f"unknown top-level keys: {', '.join(sorted(unknown))}")

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
    return memory, clocks, devices, by_name[console]


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


def generate(memory, clocks, devices, console, args):
    banner = "/* Generated by scripts/devicetree_generate.py. Do not edit. */\n"
    header = [banner, "#ifndef HOMECORE_DEVICETREE_H", "#define HOMECORE_DEVICETREE_H", ""]
    header.append(f"#define DT_CPU_CLOCK_HZ {clocks['cpu']}U")
    console_path = console["values"].get("devname")
    header.append(f"#define DT_CHOSEN_CONSOLE_PATH {c_literal(console_path)}")
    header.append(f"#define DT_DEVICE_COUNT {len(devices)}U")
    header += ["", "/* Initialize every enabled device, in description order. */",
               "void dt_init(void);", "", "#endif /* HOMECORE_DEVICETREE_H */", ""]

    source = [banner, '#include "homecore/devicetree.h"', '#include "homecore/drivers/console.h"']
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
    source += ["}", ""]

    memory_lines = [f"    {region} ({attributes}) : ORIGIN = 0x{base:08X}, LENGTH = {size}"
                    for region, (attributes, base, size) in memory.items()]

    sources = sorted({str(device["binding"]["directory"] / src)
                      for device in devices for src in device["binding"].get("sources", [])})
    directories = sorted({str(device["binding"]["directory"]) for device in devices})
    cmake = ["# Generated by scripts/devicetree_generate.py. Do not edit.",
             "set(HOMECORE_DT_SOURCES", *[f"    {s}" for s in sources], ")",
             "set(HOMECORE_DT_INCLUDE_DIRS", *[f"    {d}" for d in directories], ")", ""]

    write_if_changed(args.header, "\n".join(header))
    write_if_changed(args.source, "\n".join(source))
    write_if_changed(args.memory, "\n".join(memory_lines) + "\n")
    write_if_changed(args.cmake, "\n".join(cmake))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--soc", required=True, help="SoC description (soc.yaml)")
    parser.add_argument("--board", required=True, help="board description (board.yaml)")
    parser.add_argument("--overlay", action="append", default=[], help="overlay, applied in order")
    parser.add_argument("--bindings", required=True, help="directory searched for bindings")
    parser.add_argument("--header", required=True)
    parser.add_argument("--source", required=True)
    parser.add_argument("--memory", required=True)
    parser.add_argument("--cmake", required=True)
    args = parser.parse_args()
    try:
        description = load(args.soc)
        for path in [args.board, *args.overlay]:
            description = merge(description, load(path), Path(path).name)
        bindings = load_bindings(args.bindings)
        generate(*resolve(description, bindings), args)
    except DescriptionError as error:
        print(f"devicetree: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
