"""Tests for scripts/devicetree_generate.py; run with the project venv's Python."""
import importlib.util
import io
import tempfile
import textwrap
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("devicetree_generate",
                                              ROOT / "scripts/devicetree_generate.py")
generator = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(generator)

SOC = """
memory:
  flash: {base: 0x08000000, size: 128K}
  ram: {base: 0x20000000, size: 8K}
devices:
  usart1: {compatible: "st,stm32-usart", reg: 0x40013800, irq: 37, bus: apb2, status: disabled}
  usart2: {compatible: "st,stm32-usart", reg: 0x40004400, irq: 38, bus: apb1, status: disabled}
"""
BOARD = """
clocks: {cpu: 24000000, apb1: 24000000, apb2: 24000000}
devices:
  usart1: {status: okay, devname: uart0}
chosen: {console: usart1}
"""


class DevicetreeTest(unittest.TestCase):
    def run_generator(self, soc=SOC, board=BOARD, overlays=()):
        """Returns (exit status, stderr, outputs by name)."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            files = {"soc": soc, "board": board}
            files.update({f"overlay{i}": text for i, text in enumerate(overlays)})
            for name, text in files.items():
                (root / f"{name}.yaml").write_text(textwrap.dedent(text))
            outputs = {name: root / "out" / name for name in ("h", "c", "ld", "cmake")}
            argv = ["devicetree_generate.py", "--soc", str(root / "soc.yaml"),
                    "--board", str(root / "board.yaml"), "--bindings", str(ROOT / "src/drivers"),
                    "--header", str(outputs["h"]), "--source", str(outputs["c"]),
                    "--memory", str(outputs["ld"]), "--cmake", str(outputs["cmake"])]
            for i in range(len(overlays)):
                argv += ["--overlay", str(root / f"overlay{i}.yaml")]
            stderr = io.StringIO()
            with patch("sys.argv", argv), redirect_stderr(stderr):
                status = generator.main()
            texts = {name: path.read_text() for name, path in outputs.items() if path.exists()}
            return status, stderr.getvalue(), texts

    def assert_rejected(self, message, **kwargs):
        status, stderr, _ = self.run_generator(**kwargs)
        self.assertEqual(status, 1, stderr)
        self.assertIn(message, stderr)

    def test_valid_description(self):
        status, stderr, out = self.run_generator()
        self.assertEqual(status, 0, stderr)
        self.assertIn("#define DT_CPU_CLOCK_HZ 24000000U", out["h"])
        self.assertIn('#define DT_CHOSEN_CONSOLE_PATH "/dev/uart0"', out["h"])
        self.assertIn(".base = 0x40013800U", out["c"])
        self.assertIn(".clock_hz = 24000000U", out["c"])
        self.assertIn(".baud = 115200U", out["c"])  # binding default
        self.assertNotIn("usart2", out["c"])       # disabled: no instance
        self.assertIn("RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 8192", out["ld"])
        self.assertIn("stm32_usart.c", out["cmake"])

    def test_overlay_applies_last(self):
        status, stderr, out = self.run_generator(overlays=["devices: {usart1: {baud: 9600}}"])
        self.assertEqual(status, 0, stderr)
        self.assertIn(".baud = 9600U", out["c"])

    def test_disabled_devices_compile_no_driver(self):
        soc = SOC + '  uart9: {compatible: "ti,stellaris-uart", reg: 0x4000C000, irq: 5, status: disabled}\n'
        status, stderr, out = self.run_generator(soc=soc)
        self.assertEqual(status, 0, stderr)
        self.assertNotIn("stellaris_uart.c", out["cmake"])

    def test_rejections(self):
        cases = {
            "missing required property reg": SOC.replace("reg: 0x40013800, ", ""),
            "no binding for 'vendor,unknown'": SOC.replace('"st,stm32-usart", reg: 0x40013800',
                                                            '"vendor,unknown", reg: 0x40013800'),
        }
        for message, soc in cases.items():
            with self.subTest(message):
                self.assert_rejected(message, soc=soc)
        board_cases = {
            "unknown properties speed": BOARD.replace("devname: uart0", "speed: 1"),
            "must be at least 1": BOARD.replace("devname: uart0", "baud: 0"),
            "clock 'apb2' is not in the board's clocks": BOARD.replace(", apb2: 24000000", ""),
            "'usart2' is not an enabled device": BOARD.replace("console: usart1", "console: usart2"),
            "clocks.cpu": BOARD.replace("cpu: 24000000, ", ""),
            "device names use a-z": BOARD.replace("devname: uart0", "devname: Uart-0"),
        }
        for message, board in board_cases.items():
            with self.subTest(message):
                self.assert_rejected(message, board=board)

    def test_duplicates_rejected(self):
        both = BOARD.replace("usart1: {status: okay, devname: uart0}",
                             "usart1: {status: okay, devname: uart0}\n  usart2: {status: okay}")
        self.assert_rejected("also used by", board=both.replace("usart2: {status: okay}",
                                                                 "usart2: {status: okay, devname: uart0}"))
        self.assert_rejected("also used by", soc=SOC.replace("0x40004400", "0x40013800"),
                             board=both)

    def test_memory_rejected(self):
        self.assert_rejected("memory.ram", soc=SOC.replace("  ram: {base: 0x20000000, size: 8K}\n", ""))
        self.assert_rejected("memory.ram.base: expected an address",
                             soc=SOC.replace("base: 0x20000000", 'base: "0x20000000"'))
        self.assert_rejected("memory.ram.size", soc=SOC.replace("size: 8K", "size: 8X"))

    def test_repository_descriptions_generate(self):
        boards = {"lm3s6965evb": "ti/lm3s6965", "stm32f4discovery": "st/stm32f407",
                  "stm32vldiscovery": "st/stm32f100"}
        for board, soc in boards.items():
            with self.subTest(board):
                status, stderr, _ = self.run_generator(
                    soc=(ROOT / f"src/soc/{soc}/soc.yaml").read_text(),
                    board=(ROOT / f"src/board/{board}/board.yaml").read_text())
                self.assertEqual(status, 0, stderr)


if __name__ == "__main__":
    unittest.main()
