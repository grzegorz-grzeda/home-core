"""Build lm3s6965evb, then run from the repository root with python3."""
import re
import select
import subprocess
import time

process = subprocess.Popen(
    ["qemu-system-arm", "-M", "lm3s6965evb", "-kernel",
     "build/lm3s6965evb/homecore", "-display", "none", "-monitor", "none",
     "-serial", "stdio"],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
)

def prompt():
    output = b""
    deadline = time.monotonic() + 5
    while time.monotonic() < deadline:
        if select.select([process.stdout], [], [], 0.05)[0]:
            chunk = process.stdout.read1(4096)
            if not chunk:
                raise AssertionError(output)
            output += chunk
            if output.endswith(b"$ "):
                return output
    raise AssertionError(("shell timeout", output))

def command(text):
    process.stdin.write(text.encode() + b"\r")
    process.stdin.flush()
    return prompt().decode()

try:
    prompt()
    assert "dev/\n" in command("ls")
    devices = command("ls /dev")
    for name in ("uart0", "uart1", "uart2", "uptime"):
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
    print("PASS: QEMU ls, mkdir, cat, multiple operands, errors, descriptor reuse")
finally:
    process.terminate()
    process.wait(timeout=3)
