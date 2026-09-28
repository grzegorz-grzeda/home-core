"""Regression tests for the quality runner, using only the Python standard library."""
import argparse
from contextlib import redirect_stdout
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location(
    "check_quality", Path(__file__).resolve().parents[1] / "scripts/check_quality.py"
)
quality = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(quality)


def arguments(**overrides):
    values = dict(verbose=False, timeout=5, clang_format="clang-format",
                  python=sys.executable, cc="cc", checks=["format"])
    values.update(overrides)
    return argparse.Namespace(**values)


class QualityRunnerTests(unittest.TestCase):
    def test_discovery_includes_untracked_files_and_excludes_vendor(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            paths = ["src/new.c", "include/new.h", "tests/new.c",
                     "src/soc/ti/lm3s6965/soc_cmsis.h", "external/lib/a.c",
                     "build/generated.c", "src/readme.md"]
            for name in paths:
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.touch()
            found = {p.relative_to(root).as_posix() for p in quality.source_files(root)}
            self.assertEqual(found, {"src/new.c", "include/new.h", "tests/new.c"})

    def test_command_exit_and_diagnostics(self):
        passed, output = quality.execute(
            [sys.executable, "-c", "print('diagnostic'); raise SystemExit(7)"], quality.ROOT, 5
        )
        self.assertFalse(passed)
        self.assertIn("Exit status 7", output)
        self.assertIn("diagnostic", output)

    def test_command_success(self):
        passed, output = quality.execute(
            [sys.executable, "-c", "print('ok')"], quality.ROOT, 5
        )
        self.assertTrue(passed)
        self.assertEqual(output.strip(), "ok")

    def test_missing_tool_is_failure(self):
        with tempfile.TemporaryDirectory() as temp:
            passed, _ = quality.execute([Path(temp) / "missing-tool"], temp, 5)
            self.assertFalse(passed)

    def test_timeout_is_failure(self):
        passed, output = quality.execute(
            [sys.executable, "-c", "import time; time.sleep(30)"], quality.ROOT, 0.1
        )
        self.assertFalse(passed)
        self.assertIn("Timed out", output)

    def test_failed_configure_does_not_test_stale_firmware(self):
        runner = quality.QualityRunner(arguments(checks=["qemu"]))
        with patch.object(quality, "execute", return_value=(False, "configure failed")) as run:
            with redirect_stdout(io.StringIO()):
                result = runner.run()
            self.assertEqual(result, 1)
            self.assertEqual(run.call_count, len(quality.QEMU_TARGETS) * len(quality.CONFIGURATIONS))
            self.assertTrue(all("-S" in call.args[0] for call in run.call_args_list))

    def test_wrong_formatter_version_fails(self):
        runner = quality.QualityRunner(arguments())
        with patch.object(quality, "execute", return_value=(True, "clang-format version 18.1.0")):
            with redirect_stdout(io.StringIO()):
                self.assertEqual(runner.run(), 1)
        self.assertEqual(runner.results, [("formatter prerequisite", False)])

    def test_selected_checks_continue_after_failure(self):
        runner = quality.QualityRunner(arguments(checks=["format", "host"]))
        with patch.object(runner, "formatting", side_effect=lambda: runner.record("format", False)):
            with patch.object(runner, "host_tests", side_effect=lambda: runner.record("host", True)):
                with redirect_stdout(io.StringIO()) as output:
                    self.assertEqual(runner.run(), 1)
        self.assertEqual(runner.results, [("format", False), ("host", True)])
        self.assertIn("Partial run", output.getvalue())

    def test_selected_success_returns_zero(self):
        runner = quality.QualityRunner(arguments())
        with patch.object(runner, "formatting", side_effect=lambda: runner.record("format", True)):
            with redirect_stdout(io.StringIO()):
                self.assertEqual(runner.run(), 0)

    def test_invalid_timeouts(self):
        for value in ("0", "-1", "nan", "inf"):
            with self.subTest(value=value), self.assertRaises(argparse.ArgumentTypeError):
                quality.positive_seconds(value)


if __name__ == "__main__":
    unittest.main()
