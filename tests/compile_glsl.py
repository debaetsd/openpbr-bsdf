#!/usr/bin/env python3
"""GLSL compilation test for OpenPBR (glslangValidator, Vulkan GLSL 4.60).

Compiles tests/glsl/test_compute.comp with the default settings and with each
of the opt-in options in openpbr_settings.h. GLSL is the strictest of the
backends about types - no implicit int-to-bool conversion, no ternaries on
structs, no variadic macros - so it catches mistakes the other compilers let
through.

Usage:
    python tests/compile_glsl.py                       # glslangValidator from PATH or the Vulkan SDK
    python tests/compile_glsl.py --glslang <path>      # a specific glslangValidator

Exit code is 0 only when every case compiled.
"""

from __future__ import annotations

import argparse
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
SHADER = REPO_ROOT / "tests" / "glsl" / "test_compute.comp"

CASES = [
    ("default", []),
    ("reevaluate", ["-DOPENPBR_SAMPLE_REEVALUATE=1"]),
    ("skip-inactive", ["-DOPENPBR_SKIP_INACTIVE_LOBES=1"]),
    ("both", ["-DOPENPBR_SAMPLE_REEVALUATE=1", "-DOPENPBR_SKIP_INACTIVE_LOBES=1"]),
]


def find_glslang(explicit: str | None) -> str | None:
    if explicit:
        return explicit
    found = shutil.which("glslangValidator")
    if found:
        return found
    sdk = os.environ.get("VULKAN_SDK")
    if sdk:
        for sub in ("Bin", "bin"):
            candidate = Path(sdk) / sub / ("glslangValidator.exe" if os.name == "nt" else "glslangValidator")
            if candidate.exists():
                return str(candidate)
    return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--glslang", help="path to glslangValidator")
    args = parser.parse_args()

    glslang = find_glslang(args.glslang)
    if not glslang:
        print("glslangValidator not found (pass --glslang, put it on PATH, or set VULKAN_SDK)", file=sys.stderr)
        return 2

    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        for name, defines in CASES:
            out = Path(tmp) / f"{name}.spv"
            cmd = [glslang, "-V", "--target-env", "vulkan1.2", "-I" + str(REPO_ROOT), *defines, "-o", str(out), str(SHADER)]
            result = subprocess.run(cmd, capture_output=True, text=True)
            ok = result.returncode == 0
            failures += 0 if ok else 1
            print(f"  {'PASS' if ok else 'FAIL'}  glsl/{name}")
            if not ok:
                print((result.stdout + result.stderr).strip())
    print(f"\n{len(CASES) - failures}/{len(CASES)} passed")
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
