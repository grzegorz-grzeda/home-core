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
arch: test/cpu
memory:
  flash: {base: 0x08000000, size: 128K}
  ram: {base: 0x20000000, size: 8K}
devices:
  usart1: {compatible: "st,stm32-usart", reg: 0x40013800, irq: 37, bus: apb2, status: disabled}
  usart2: {compatible: "st,stm32-usart", reg: 0x40004400, irq: 38, bus: apb1, status: disabled}
"""
BOARD = """
soc: test/chip
clocks: {cpu: 24000000, apb1: 24000000, apb2: 24000000}
devices:
  usart1: {status: okay, devname: uart0}
chosen: {console: usart1}
"""


class DevicetreeTest(unittest.TestCase):
    def run_generator(self, soc=SOC, board=BOARD, overlays=(), board_ld=False, real_board=None):
        """Returns (exit status, stderr, outputs by name). real_board runs a repository board."""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            soc_root, arch_root = root / "soc", root / "arch"
            (soc_root / "test/chip").mkdir(parents=True)
            (soc_root / "test/chip/soc.yaml").write_text(textwrap.dedent(soc))
            (arch_root / "test/cpu").mkdir(parents=True)
            (arch_root / "test/cpu/arch.cmake").write_text("")
            board_path = root / "myboard/board.yaml"
            board_path.parent.mkdir()
            board_path.write_text(textwrap.dedent(board))
            if board_ld:
                (board_path.parent / "board.ld").write_text("/* board */\n")
            if real_board:
                board_path = ROOT / f"src/board/{real_board}/board.yaml"
                soc_root, arch_root = ROOT / "src/soc", ROOT / "src/arch"
            outputs = {name: root / "out" / name for name in ("h", "c", "ld", "cmake")}
            argv = ["devicetree_generate.py", "--board", str(board_path),
                    "--soc-root", str(soc_root), "--arch-root", str(arch_root),
                    "--bindings", str(ROOT / "src/drivers"),
                    "--header", str(outputs["h"]), "--source", str(outputs["c"]),
                    "--memory", str(outputs["ld"]), "--cmake", str(outputs["cmake"])]
            for i, text in enumerate(overlays):
                overlay = root / f"overlay{i}.yaml"
                overlay.write_text(textwrap.dedent(text))
                argv += ["--overlay", str(overlay)]
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
        self.assertIn("case 37U:\n        stm32_usart_isr(&dt_usart1);", out["c"])
        self.assertIn('board_panic("Unexpected interrupt");', out["c"])
        self.assertIn('set(HOMECORE_SOC "test/chip")', out["cmake"])
        self.assertIn('set(HOMECORE_ARCH "test/cpu")', out["cmake"])
        self.assertIn('#define DT_BOARD_NAME "myboard"', out["h"])  # directory name default
        self.assertIn("#define DT_CLOCK_SETUP 1", out["h"])
        self.assertIn("empty.ld", out["cmake"])  # no soc.ld or board.ld

    def test_selection(self):
        status, stderr, out = self.run_generator(board="name: MY_BOARD\n" + BOARD, board_ld=True)
        self.assertEqual(status, 0, stderr)
        self.assertIn('#define DT_BOARD_NAME "MY_BOARD"', out["h"])
        self.assertRegex(out["cmake"], r'HOMECORE_BOARD_LINKER_SCRIPT ".*myboard/board.ld"')
        self.assert_rejected("unknown SoC 'test/other'", board=BOARD.replace("test/chip", "test/other"))
        self.assert_rejected("'soc' must name a SoC", board=BOARD.replace("soc: test/chip\n", ""))
        self.assert_rejected("'soc' must name a SoC", board=BOARD.replace("test/chip", "../chip"))
        self.assert_rejected("unknown architecture 'test/gpu'", soc=SOC.replace("test/cpu", "test/gpu"))
        self.assert_rejected("not allowed in a board description: arch",
                             board="arch: test/cpu\n" + BOARD)
        self.assert_rejected("not allowed in a soc description: clocks",
                             soc="clocks: {cpu: 1}\n" + SOC)
        self.assert_rejected("not allowed in a overlay description: soc",
                             overlays=["soc: test/chip"])

    def test_overlay_applies_last(self):
        status, stderr, out = self.run_generator(overlays=["devices: {usart1: {baud: 9600}}",
                                                           "clock_setup: false"])
        self.assertEqual(status, 0, stderr)
        self.assertIn(".baud = 9600U", out["c"])
        self.assertIn("#define DT_CLOCK_SETUP 0", out["h"])
        self.assert_rejected("clock_setup: expected true or false", overlays=["clock_setup: 0"])
        # clock_setup is not a clock: a device cannot name it as its bus.
        self.assert_rejected("clock '_setup'", board=BOARD.replace("devname: uart0", "bus: _setup"))

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

    def test_mounts(self):
        board = BOARD.replace("chosen:", '  ram0: {compatible: "homecore,ramdisk", size: 64K}\nchosen:')
        mount = "mounts:\n  /ram: {device: ram0, fs: fat, format: true}\n"
        status, stderr, out = self.run_generator(board=board + mount)
        self.assertEqual(status, 0, stderr)
        self.assertIn("static uint8_t dt_ram0_data[65536] __attribute__((aligned(4)));", out["c"])
        self.assertIn(".data = dt_ram0_data,", out["c"])
        self.assertIn(".size = 65536U,", out["c"])
        self.assertIn(".ops = &ramdisk_block_ops,", out["c"])
        self.assertIn('fs_fat_mount(&dt_ram0_block, "/ram", true) < 0', out["c"])
        self.assertIn('#include "homecore/fs/fat.h"', out["c"])
        self.assertIn("#define DT_MOUNT_COUNT 1U", out["h"])
        self.assertIn('set(HOMECORE_DT_FILESYSTEMS "fat")', out["cmake"])
        self.assertIn("ramdisk.c", out["cmake"])
        status, stderr, out = self.run_generator(board=board + mount.replace("fat", "littlefs"))
        self.assertEqual(status, 0, stderr)
        self.assertIn('fs_littlefs_mount(&dt_ram0_block, "/ram", true) < 0', out["c"])
        self.assertIn('set(HOMECORE_DT_FILESYSTEMS "littlefs")', out["cmake"])
        # Overlays can add mounts; no mounts compile no filesystem.
        status, stderr, out = self.run_generator(board=board, overlays=[mount])
        self.assertEqual(status, 0, stderr)
        self.assertIn('"/ram", true', out["c"])
        status, stderr, out = self.run_generator()
        self.assertEqual(status, 0, stderr)
        self.assertIn('set(HOMECORE_DT_FILESYSTEMS "")', out["cmake"])
        self.assertIn("void dt_mount_all(void) {\n}", out["c"])
        cases = {
            "must be a multiple of 512": (board.replace("64K", "1000"), mount),
            "must be at least 512": (board.replace("64K", "0"), mount),
            "invalid size": (board.replace("64K", "true"), mount),
            "'usart1' is not an enabled block device": (board, mount.replace("ram0", "usart1")),
            "'ram9' is not an enabled block device": (board, mount.replace("ram0", "ram9")),
            "mounts./a/b: mount points are top-level": (board, mount.replace("/ram", "/a/b")),
            "mounts./dev: mount points": (board, mount.replace("/ram", "/dev")),
            "fs: expected one of fat": (board, mount.replace("fs: fat", "fs: ext4")),
            "format: expected true or false": (board, mount.replace("true", "1")),
            "allowed keys are": (board, mount.replace("format", "mode")),
            "'ram0' is already mounted": (board, mount + "  /two: {device: ram0, fs: fat}\n"),
            "expected a mapping of mount-point paths": (board, "mounts: [/ram]\n"),
        }
        for message, (text, mounts) in cases.items():
            with self.subTest(message):
                self.assert_rejected(message, board=text + mounts)

    def test_repository_descriptions_generate(self):
        for board in ("lm3s6965evb", "stm32f4discovery", "stm32vldiscovery"):
            with self.subTest(board):
                status, stderr, out = self.run_generator(real_board=board)
                self.assertEqual(status, 0, stderr)
                self.assertIn(f'#define DT_BOARD_NAME "{board.upper()}"', out["h"])


if __name__ == "__main__":
    unittest.main()
