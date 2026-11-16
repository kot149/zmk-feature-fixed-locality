#!/usr/bin/env python3
"""Compile the real behavior against external API doubles for each ZMK role."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
variants = {
    "standalone": (0, 0, 0),
    "central": (1, 1, 3),
    "peripheral": (1, 0, 3),
    "central-no-peers": (1, 1, 0),
}
with tempfile.TemporaryDirectory(prefix="zmk-fixed-locality-tests-") as temp:
    for name, (split, central, count) in variants.items():
        binary = Path(temp) / name
        subprocess.run([
            *shlex.split(os.environ.get("CC", "cc")), "-std=c11", "-Wall", "-Wextra",
            "-Werror", "-Wno-unused-parameter", "-DCONFIG_ZMK_BEHAVIOR_METADATA=1",
            f"-DCONFIG_ZMK_SPLIT={split}", f"-DCONFIG_ZMK_SPLIT_ROLE_CENTRAL={central}",
            f"-DZMK_SPLIT_CENTRAL_PERIPHERAL_COUNT={count}",
            "-I", str(root / "tests/stubs"), "-I", str(root / "dts"),
            str(root / "tests/test_fixed_locality.c"), "-o", str(binary),
        ], check=True)
        print(f"{name}: ", end="", flush=True)
        subprocess.run([str(binary)], check=True)
