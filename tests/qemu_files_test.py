"""Build the board's QEMU configuration, then run from the repository root with python3."""
import argparse
import re
import select
import subprocess
import time

# QEMU machine, default firmware, console devices the SoC registers, and the
# BASIC nesting limit from the board's defconfig.
BOARDS = {
    "lm3s6965evb": ("lm3s6965evb", "build/lm3s6965evb/homecore",
                    ("uart0", "uart1", "uart2"), 8),
    "stm32vldiscovery": ("stm32vldiscovery", "build/stm32vldiscovery-qemu/homecore",
                         ("uart0",), 4),
}

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--board", choices=sorted(BOARDS), default="lm3s6965evb",
                    help="board to emulate (default: lm3s6965evb)")
parser.add_argument("--firmware", help="firmware ELF to test (default: the board's preset build)")
args = parser.parse_args()
machine, default_firmware, uarts, nesting = BOARDS[args.board]

process = subprocess.Popen(
    ["qemu-system-arm", "-M", machine, "-kernel",
     args.firmware or default_firmware, "-display", "none", "-monitor", "none",
     "-serial", "stdio"],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
)

def prompt(ending=b"$ "):
    output = b""
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if select.select([process.stdout], [], [], 0.05)[0]:
            chunk = process.stdout.read1(4096)
            if not chunk:
                raise AssertionError(output)
            output += chunk
            if output.endswith(ending):
                return output
    raise AssertionError(("shell timeout", output))

def command(text, ending=b"$ "):
    process.stdin.write(text.encode() + b"\r")
    process.stdin.flush()
    return prompt(ending).decode()

try:
    prompt()
    assert "dev/\n" in command("ls")
    devices = command("ls /dev")
    for name in uarts + ("uptime",):
        assert name + "\n" in devices, devices
    assert command("mkdir /tmp").endswith("$ ")
    assert "tmp/\n" in command("ls /")
    command("mkdir /tmp/one /tmp/two")
    listing = command("ls /tmp")
    assert "one/\n" in listing and "two/\n" in listing, listing
    assert "File exists" in command("mkdir /tmp")
    assert "No such file" in command("mkdir /absent/child")
    assert "Not a directory" in command("mkdir /dev/uptime/child")
    assert "Is a directory" in command("cat /tmp")
    assert "No such file" in command("cat /missing")
    values = command("cat /dev/uptime dev/uptime")
    assert len(re.findall(r"(?m)^\d+\n", values)) == 2, values
    for _ in range(20):
        assert re.search(r"(?m)^\d+\n", command("cat /dev/uptime"))
    assert "uptime\n" in command("ls //dev/./uptime")
    for text in ("mkdir", "cat", "ls / /dev"):
        assert "Usage:" in command(text)
    assert command("whoami") == "whoami\r\nroot\nroot:/$ "
    assert "uid=0(root) gid=0\n" in command("id")
    assert command("cd /tmp").endswith("root:/tmp$ ")
    assert "/tmp\n" in command("pwd")
    command("mkdir relative")
    assert "relative/\n" in command("ls")
    assert command("cd relative").endswith("root:/tmp/relative$ ")
    assert command("cd ..").endswith("root:/tmp$ ")
    assert command("cd /missing").endswith("root:/tmp$ ")
    assert "No such file" in command("cd /missing")
    assert "Not a directory" in command("cd /dev/uptime")
    assert command("cd /dev").endswith("root:/dev$ ")
    assert re.search(r"(?m)^\d+\n", command("cat uptime"))
    assert command("cd").endswith("root:/$ ")
    assert command("cd ../../..").endswith("root:/$ ")
    for text in ("cd / /dev", "pwd extra", "whoami extra", "id extra"):
        assert "Usage:" in command(text)
    command("mkdir /scratch")
    command("cd /scratch")
    command("touch one two")
    listing = command("ls")
    assert "one\n" in listing and "two\n" in listing, listing
    assert command("cat one").endswith("root:/scratch$ ")
    assert "Device or resource busy" in command("rmdir .")
    command("cd /")
    assert "Directory not empty" in command("rmdir /scratch")
    protected = command("rm /dev/uptime")
    assert "rm: /dev/uptime:" in protected, protected
    assert "uptime\n" in command("ls /dev/uptime")
    assert "Is a directory" in command("rm /scratch")
    command("rm /scratch/one /scratch/two")
    command("rmdir /scratch")
    assert "No such file" in command("ls /scratch")
    for _ in range(20):
        command("mkdir /reused")
        command("touch /reused/file")
        command("rm /reused/file")
        command("rmdir /reused")
    # Nested function calls are the most stack-hungry BASIC construct; the mem
    # check below proves the stack held at the board's nesting limit.
    command("basic", b"> ")
    def calls(depth):
        return "PRINT " + "min(" * depth + "7" + ")" * depth
    assert re.search(r"(?m)^7$", command(calls(nesting), b"> ")), nesting
    assert "expression too deeply nested" in command(calls(nesting + 1), b"> ")
    assert "42" in command("PRINT 6 * 7", b"> ")
    process.stdin.write(b"\x03")
    process.stdin.flush()
    prompt()
    memory = command("mem")
    values = dict(re.findall(r"(Heap total|Allocated|Reusable|Unclaimed|Available): (\d+) bytes", memory))
    assert len(values) == 5, memory
    values = {key: int(value) for key, value in values.items()}
    assert values["Heap total"] > 0 and values["Allocated"] > 0, memory
    assert values["Available"] == values["Reusable"] + values["Unclaimed"], memory
    assert values["Available"] <= values["Heap total"], memory
    stack = re.search(r"Stack: used (\d+) of (\d+) bytes", memory)
    assert stack and 0 < int(stack.group(1)) < int(stack.group(2)), memory
    assert re.search(r"up \d+ days, \d{2}:\d{2}:\d{2}\.\d{3}", command("uptime"))
    assert "\x1b[2J\x1b[H" in command("clear")
    for text in ("mem extra", "uptime extra", "clear extra", "reboot extra", "touch", "rm", "rmdir"):
        assert "Usage:" in command(text)
    command("touch /volatile")
    restarted = command("reboot")
    assert "Rebooting..." in restarted and "HomeCore OS" in restarted, restarted
    assert restarted.endswith("root:/$ "), restarted
    assert "No such file" in command("ls /volatile")
    print(f"PASS: QEMU {args.board} system/file commands, sessions, create/remove cycles, "
          f"BASIC nesting limit {nesting} and reboot")
finally:
    process.terminate()
    process.wait(timeout=3)
