"""Run the examples in the user guides in QEMU and check their documented output.

Build the board's QEMU configuration first, then run from the repository root
with python3. In the guides, a fenced block tagged `homecore` holds shell
commands and one tagged `basic` holds BASIC lines. When a `text` block follows
it (with only blank lines between), the commands' output must match it
exactly. A `text <board>...` block applies only to the named boards. Blocks
run in order in one session per guide, so later examples see earlier files.
Each `basic` block starts with NEW.
"""
import argparse
import difflib
import select
import subprocess
import sys
import time
from pathlib import Path

# QEMU machine and default firmware per board.
BOARDS = {
    "lm3s6965evb": ("lm3s6965evb", "build/lm3s6965evb/homecore"),
    "stm32vldiscovery": ("stm32vldiscovery", "build/stm32vldiscovery-qemu/homecore"),
}
# Guides and the boards whose QEMU build they describe.
GUIDES = {
    "docs/tour.md": ("lm3s6965evb",),
    "docs/basic.md": ("lm3s6965evb", "stm32vldiscovery"),
}
SHELL_PROMPT = b"$ "
BASIC_PROMPT = b"> "


def fenced_blocks(path):
    """Yield (info, lines, first line number, last line number) per fenced block."""
    lines = Path(path).read_text(encoding="utf-8").splitlines()
    index = 0
    while index < len(lines):
        if lines[index].startswith("```"):
            info, start, body = lines[index][3:].strip(), index + 1, []
            index += 1
            while index < len(lines) and not lines[index].startswith("```"):
                body.append(lines[index])
                index += 1
            yield info, body, start, index + 1
        index += 1


def examples(path, board):
    """Yield (kind, lines, expected output or None, line number)."""
    blocks = list(fenced_blocks(path))
    text = Path(path).read_text(encoding="utf-8").splitlines()
    for position, (info, body, start, end) in enumerate(blocks):
        if info not in ("homecore", "basic"):
            continue
        expected, previous_end = None, end
        for next_info, next_body, next_start, next_end in blocks[position + 1:]:
            between = text[previous_end:next_start - 1]
            words = next_info.split()
            if any(line.strip() for line in between) or not words or words[0] != "text":
                break
            if len(words) == 1 or board in words[1:]:
                expected = "\n".join(next_body)
            previous_end = next_end
        yield info, body, expected, start


class Session:
    def __init__(self, machine, firmware):
        self.process = subprocess.Popen(
            ["qemu-system-arm", "-M", machine, "-kernel", firmware, "-display", "none",
             "-monitor", "none", "-serial", "stdio"],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        self.mode = "shell"
        self.read(SHELL_PROMPT)

    def read(self, ending, timeout=10):
        output = b""
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if select.select([self.process.stdout], [], [], 0.05)[0]:
                chunk = self.process.stdout.read1(4096)
                if not chunk:
                    break
                output += chunk
                if output.endswith(ending):
                    return output.decode()
        raise AssertionError(f"no prompt; output so far: {output!r}")

    def send(self, text, ending):
        """Send one line; return its output without the echo and the prompt."""
        self.process.stdin.write(text.encode() + b"\r")
        self.process.stdin.flush()
        output = self.read(ending)
        output = output.split("\r\n", 1)[1] if "\r\n" in output else ""
        return output[:output.rfind("\n") + 1] if "\n" in output else ""

    def run(self, kind, lines):
        output = ""
        if kind == "basic":
            if self.mode == "shell":
                self.send("basic", BASIC_PROMPT)
                self.mode = "basic"
            self.send("NEW", BASIC_PROMPT)
            for line in lines:
                output += self.send(line, BASIC_PROMPT)
        else:
            if self.mode == "basic":
                self.process.stdin.write(b"\x03")
                self.process.stdin.flush()
                self.read(SHELL_PROMPT)
                self.mode = "shell"
            for line in lines:
                output += self.send(line, SHELL_PROMPT)
        return output

    def close(self):
        self.process.terminate()
        self.process.wait(timeout=3)


def normalize(text):
    lines = [line.rstrip() for line in text.replace("\r\n", "\n").split("\n")]
    while lines and not lines[-1]:
        lines.pop()
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--board", choices=sorted(BOARDS), default="lm3s6965evb")
    parser.add_argument("--firmware", help="firmware ELF (default: the board's preset build)")
    args = parser.parse_args()
    machine, default_firmware = BOARDS[args.board]
    failures, checked = 0, 0
    for guide, boards in GUIDES.items():
        if args.board not in boards:
            continue
        session = Session(machine, args.firmware or default_firmware)
        try:
            for kind, lines, expected, line_number in examples(guide, args.board):
                output = session.run(kind, lines)
                if expected is None:
                    continue
                checked += 1
                if normalize(output) != normalize(expected):
                    failures += 1
                    print(f"{guide}:{line_number}: output differs from the documented output")
                    sys.stdout.writelines(difflib.unified_diff(
                        [line + "\n" for line in normalize(expected)],
                        [line + "\n" for line in normalize(output)],
                        "documented", "actual"))
        finally:
            session.close()
    if failures:
        print(f"FAIL: {failures} of {checked} documented examples differ on {args.board}")
        return 1
    print(f"PASS: {checked} documented examples match on {args.board}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
