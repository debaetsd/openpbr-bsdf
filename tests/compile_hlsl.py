#!/usr/bin/env python3
"""DXC compilation test for the OpenPBR HLSL backend (D3D12 / DXIL).

Compiles the shaders in tests/hlsl/ across a matrix of shader stages, shader
models and OpenPBR configurations, and reports which combinations build.

DXIL only - this targets D3D12. DXC's SPIR-V backend is not exercised here.

This is deliberately a plain script rather than a unit-test suite: paths and
permutations are hardcoded below, and the only dependency is a DXC binary.

Usage:
    python tests/compile_hlsl.py                  # run the full matrix
    python tests/compile_hlsl.py --list           # print the matrix, compile nothing
    python tests/compile_hlsl.py -k raytracing    # only cases matching a substring
    python tests/compile_hlsl.py --quick          # one case per shader, highest model
    python tests/compile_hlsl.py --dxc <path>     # use a specific dxc.exe
    python tests/compile_hlsl.py --werror         # treat DXC warnings as failures
    python tests/compile_hlsl.py --keep-output    # keep the compiled DXIL

Exit code is 0 only when every case that ran succeeded.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

REPO_ROOT = Path(__file__).resolve().parents[1]
SHADER_DIR = REPO_ROOT / "tests" / "hlsl"

# The DXC release this matrix is maintained against. Older builds usually work;
# the script warns rather than refuses, so you can still run it with whatever is
# installed. Pass --require-dxc-version to turn the warning into an error.
#
# Note that the release tag and the version DXC reports are two different
# numbers: the v1.9.2607 release identifies itself as build 1.9.0.5402. The tag
# is what you download, the build is what `dxc --version` prints, so the check
# has to compare against the build.
PINNED_DXC_RELEASE = "v1.9.2607"
PINNED_DXC_BUILD = "1.9.0.5402"
DXC_RELEASE_ASSET = "dxc_2026_07_29.zip"
DXC_RELEASE_URL = (
    f"https://github.com/microsoft/DirectXShaderCompiler/releases/download/{PINNED_DXC_RELEASE}/{DXC_RELEASE_ASSET}"
)

# Searched in order. The first entry that exists and runs wins, unless --dxc is
# given. Put a locally downloaded DXC first so it takes priority over the older
# copies that ship inside the Windows and Vulkan SDKs.
DXC_SEARCH_PATHS = [
    REPO_ROOT / "tools" / "dxc" / "bin" / "x64" / "dxc.exe",
    REPO_ROOT / "tools" / "dxc" / "dxc.exe",
    Path(r"C:\dxc\bin\x64\dxc.exe"),
    Path(r"C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe"),
    Path(r"C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\dxc.exe"),
    Path(r"C:\VulkanSDK\1.4.313.0\Bin\dxc.exe"),
]

# The OpenPBR HLSL backend requires HLSL 2021 semantics as a floor; the tests
# run the newest mode DXC offers so language changes surface here first.
# -HV 202x reports __HLSL_VERSION == 2029.
HLSL_VERSION = "202x"

# ---------------------------------------------------------------------------
# Matrix definition
# ---------------------------------------------------------------------------


@dataclass(frozen=True)
class Shader:
    """One test shader and how it has to be invoked."""

    name: str
    file: str
    stage: str  # DXC target-profile prefix: cs, ps, vs, lib
    entry: str | None = None  # None for lib targets, which have no single entry
    min_shader_model: tuple[int, int] = (6, 0)
    # Shader models to use instead of the default sweep, when a shader only
    # needs to prove it builds rather than to cover the whole range.
    shader_models: tuple[str, ...] | None = None
    # Restrict this shader to specific variants, for shaders that only make
    # sense under one axis of the matrix.
    only_variants: tuple[str, ...] | None = None
    note: str = ""


SHADERS = [
    Shader(
        name="compute",
        file="test_compute.hlsl",
        stage="cs",
        entry="main",
        note="default configuration, full API",
    ),
    Shader(
        name="pixel",
        file="test_pixel.hlsl",
        stage="ps",
        entry="main",
        shader_models=("6_0", "6_6"),
        note="pixel stage",
    ),
    Shader(
        name="vertex",
        file="test_vertex.hlsl",
        stage="vs",
        entry="main",
        shader_models=("6_0", "6_6"),
        note="vertex stage",
    ),
    Shader(
        name="raytracing",
        file="test_raytracing.hlsl",
        stage="lib",
        entry=None,
        min_shader_model=(6, 3),  # DXR requires lib_6_3 or newer
        shader_models=("6_3", "6_6", "6_8"),
        note="DXR library",
    ),
    Shader(
        name="texture_luts",
        file="test_texture_luts.hlsl",
        stage="cs",
        entry="main",
        shader_models=("6_6",),
        note="OPENPBR_USE_TEXTURE_LUTS=1",
    ),
    Shader(
        name="custom_hooks",
        file="test_custom_hooks.hlsl",
        stage="cs",
        entry="main",
        shader_models=("6_6",),
        note="fast-math / custom vec types / spec constants / reciprocal coat",
    ),
    Shader(
        name="autodetect",
        file="test_autodetect.hlsl",
        stage="cs",
        entry="main",
        shader_models=("6_6",),
        note="no explicit language target; selector must pick HLSL",
    ),
    Shader(
        name="make_struct",
        file="test_make_struct.hlsl",
        stage="cs",
        entry="main",
        shader_models=("6_6",),
        note="all 15 OPENPBR_MAKE_STRUCT_N arities + OPENPBR_STATIC_ASSERT",
    ),
    Shader(
        name="spec_constants",
        file="test_spec_constants.hlsl",
        stage="cs",
        entry="main",
        shader_models=("6_6",),
        only_variants=tuple("perm-%d%d%d%d" % (s, d, t, m)
                            for s in (0, 1) for d in (0, 1) for t in (0, 1) for m in (0, 1)),
        note="OPENPBR_GET_SPECIALIZATION_CONSTANT as compile-time permutations",
    ),
]

# The four feature toggles OPENPBR_GET_SPECIALIZATION_CONSTANT can be asked for,
# in the bit order used by the perm-SDTM variant names below.
SPECIALIZATION_TOGGLES = (
    "EnableSheenAndCoat",
    "EnableDispersion",
    "EnableTranslucency",
    "EnableMetallic",
)

# Default shader-model sweep for shaders that do not pin their own.
DEFAULT_SHADER_MODELS = ("6_0", "6_2", "6_5", "6_6", "6_7")


@dataclass(frozen=True)
class Variant:
    """An axis of the matrix that is independent of which shader is compiled."""

    name: str
    args: tuple[str, ...] = ()
    min_shader_model: tuple[int, int] = (6, 0)
    # Restrict this variant to a few shaders, to keep the matrix manageable.
    only_shaders: tuple[str, ...] | None = None


VARIANTS = [
    Variant(name="default", args=()),
    # -enable-16bit-types switches the energy tables to uint16_t storage, which
    # is a different code path through impl/data/openpbr_energy_arrays.h.
    Variant(name="16bit", args=("-enable-16bit-types",), min_shader_model=(6, 2)),
    # Optimization levels only on the baseline shader: -O0 keeps the IR closest
    # to what the front end produced, -O3 is what shipping builds use.
    Variant(name="O0", args=("-O0",), only_shaders=("compute",)),
    Variant(name="O3", args=("-O3",), only_shaders=("compute",)),
    # -Zi forces debug info through the DXIL writer, which has its own failure
    # modes around large constant arrays.
    Variant(name="debug", args=("-Zi", "-Qembed_debug"), only_shaders=("compute",)),
]

# One variant per feature-toggle combination, the way a D3D12 renderer consumes
# OPENPBR_GET_SPECIALIZATION_CONSTANT: a -D per toggle, one shader permutation
# per combination. The name is perm-SDTM, one bit per SPECIALIZATION_TOGGLES
# entry in order, so perm-1111 is everything on and perm-0000 everything off.
VARIANTS += [
    Variant(
        name="perm-%d%d%d%d" % bits,
        args=tuple(
            "-DOPENPBR_PERMUTATION_%s=%d" % (toggle, bit)
            for toggle, bit in zip(SPECIALIZATION_TOGGLES, bits)
        ),
        only_shaders=("spec_constants",),
    )
    for bits in [(s, d, t, m) for s in (0, 1) for d in (0, 1) for t in (0, 1) for m in (0, 1)]
]


@dataclass
class Case:
    """A single dxc invocation."""

    shader: Shader
    variant: Variant
    shader_model: str

    @property
    def profile(self) -> str:
        return f"{self.shader.stage}_{self.shader_model}"

    @property
    def name(self) -> str:
        return f"{self.shader.name}/{self.profile}/{self.variant.name}"

    def command(self, dxc: Path, output: Path | None) -> list[str]:
        cmd = [str(dxc), "-T", self.profile, "-HV", HLSL_VERSION]
        if self.shader.entry:
            cmd += ["-E", self.shader.entry]
        cmd += list(self.variant.args)
        cmd += ["-I", str(REPO_ROOT), "-I", str(SHADER_DIR)]
        if output is not None:
            cmd += ["-Fo", str(output)]
        cmd.append(str(SHADER_DIR / self.shader.file))
        return cmd


def parse_shader_model(sm: str) -> tuple[int, int]:
    major, minor = sm.split("_")
    return int(major), int(minor)


def build_matrix(quick: bool) -> list[Case]:
    cases: list[Case] = []
    for shader in SHADERS:
        for variant in VARIANTS:
            if variant.only_shaders and shader.name not in variant.only_shaders:
                continue
            if shader.only_variants and variant.name not in shader.only_variants:
                continue
            if quick and variant.name != "default":
                continue

            models = shader.shader_models or DEFAULT_SHADER_MODELS
            if quick:
                models = (models[-1],)

            for sm in models:
                parsed = parse_shader_model(sm)
                if parsed < shader.min_shader_model:
                    continue
                if parsed < variant.min_shader_model:
                    continue
                cases.append(Case(shader=shader, variant=variant, shader_model=sm))
    return cases


# ---------------------------------------------------------------------------
# DXC discovery
# ---------------------------------------------------------------------------


@dataclass
class DxcInfo:
    path: Path
    version: str
    raw_version: str


def query_dxc_version(dxc: Path) -> str | None:
    try:
        result = subprocess.run(
            [str(dxc), "--version"],
            capture_output=True,
            text=True,
            timeout=30,
        )
    except (OSError, subprocess.SubprocessError):
        return None
    output = (result.stdout + result.stderr).strip()
    return output or None


def normalize_version(raw: str) -> str:
    """Pull the build number out of a `dxc --version` banner.

    The banner takes a few shapes depending on the build:
        dxcompiler.dll: 1.8 - 1.8.2502.11 (239921522); dxil.dll: 1.8(1.8.2502.11)
        dxcompiler.dll: 1.9 - 1.8.0.4907 (06381f2d7)
        dxcompiler.dll: 1.9(5402-0d3ee6b5)(1.9.0.5402) - 1.9.0.5402 (0d3ee6b55-dirty)
    In all of them the four-component number after the dash is the build, which
    is what PINNED_DXC_BUILD is compared against.
    """
    match = re.search(r"-\s*(\d+\.\d+\.\d+\.\d+)", raw)
    if match:
        return match.group(1)
    match = re.search(r"(\d+\.\d+\.\d+\.\d+)", raw)
    if match:
        return match.group(1)
    return raw.strip()


def find_dxc(explicit: str | None) -> DxcInfo:
    candidates: list[Path] = []
    if explicit:
        candidates = [Path(explicit)]
    else:
        env = os.environ.get("OPENPBR_DXC")
        if env:
            candidates.append(Path(env))
        candidates += DXC_SEARCH_PATHS
        on_path = shutil.which("dxc")
        if on_path:
            candidates.append(Path(on_path))

    tried: list[str] = []
    for candidate in candidates:
        if not candidate.exists():
            tried.append(f"  {candidate}  (not found)")
            continue
        raw = query_dxc_version(candidate)
        if raw is None:
            tried.append(f"  {candidate}  (failed to run)")
            continue
        return DxcInfo(
            path=candidate,
            version=normalize_version(raw),
            raw_version=raw.splitlines()[0],
        )

    sys.exit(
        "error: no usable dxc.exe found. Tried:\n"
        + "\n".join(tried)
        + f"\n\nPass --dxc <path>, set OPENPBR_DXC, or download DXC {PINNED_DXC_RELEASE}:\n"
        f"  {DXC_RELEASE_URL}\n"
        f"and unpack it to {REPO_ROOT / 'tools' / 'dxc'} (so dxc.exe lands in tools/dxc/bin/x64/)."
    )


# ---------------------------------------------------------------------------
# Running
# ---------------------------------------------------------------------------


@dataclass
class Result:
    case: Case
    ok: bool
    returncode: int
    duration: float
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    output_size: int = 0
    raw_output: str = ""


DIAGNOSTIC_RE = re.compile(r"^(?P<file>.+?):(?P<line>\d+):(?P<col>\d+): (?P<level>error|warning|fatal error): (?P<msg>.*)$")


def extract_diagnostics(text: str) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: list[str] = []
    for line in text.splitlines():
        match = DIAGNOSTIC_RE.match(line.strip())
        if not match:
            # Some DXC failures (bad profile, missing file) have no location.
            stripped = line.strip()
            if stripped.startswith("error:") or stripped.startswith("fatal error:"):
                errors.append(stripped)
            continue
        where = f"{Path(match['file']).name}:{match['line']}"
        entry = f"{where}: {match['msg']}"
        if match["level"] == "warning":
            warnings.append(entry)
        else:
            errors.append(entry)
    return errors, warnings


def run_case(case: Case, dxc: Path, out_dir: Path | None, werror: bool) -> Result:
    if out_dir is not None:
        safe = case.name.replace("/", "__")
        output: Path | None = out_dir / f"{safe}.dxil"
    else:
        # Still write to a real file - some DXC versions behave differently with
        # no -Fo at all - but into a directory that gets thrown away.
        output = None

    started = time.perf_counter()
    with tempfile.TemporaryDirectory() as tmp:
        if output is None:
            output = Path(tmp) / "out.dxil"
        cmd = case.command(dxc, output)
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
            combined = proc.stdout + proc.stderr
            returncode = proc.returncode
        except subprocess.TimeoutExpired:
            return Result(
                case=case,
                ok=False,
                returncode=-1,
                duration=time.perf_counter() - started,
                errors=["dxc timed out after 600s"],
            )
        size = output.stat().st_size if output.exists() else 0

    duration = time.perf_counter() - started
    errors, warnings = extract_diagnostics(combined)
    ok = returncode == 0 and size > 0
    if werror and warnings:
        ok = False
    return Result(
        case=case,
        ok=ok,
        returncode=returncode,
        duration=duration,
        errors=errors,
        warnings=warnings,
        output_size=size,
        raw_output=combined,
    )


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

GREEN = "\033[32m"
RED = "\033[31m"
YELLOW = "\033[33m"
DIM = "\033[2m"
RESET = "\033[0m"


def supports_color() -> bool:
    if os.environ.get("NO_COLOR"):
        return False
    return sys.stdout.isatty()


def check_specialization_toggles(results: list[Result]) -> list[str]:
    """Verify the feature toggles actually change what gets compiled.

    A permutation shader whose toggles are silently ignored still compiles 16
    times and still reports 16 passes, so "it built" proves nothing on its own.
    Disabling a toggle removes a lobe, so every single-toggle-on build must be
    strictly larger than the all-off build. If that does not hold, the -D is not
    reaching OPENPBR_GET_SPECIALIZATION_CONSTANT and this whole axis of the
    matrix is vacuous.
    """
    sizes = {
        r.case.variant.name: r.output_size
        for r in results
        if r.case.shader.name == "spec_constants" and r.ok
    }
    all_off = sizes.get("perm-0000")
    if all_off is None:
        return []  # filtered out, or it failed and is already reported as such

    problems = []
    for index, toggle in enumerate(SPECIALIZATION_TOGGLES):
        bits = ["0"] * len(SPECIALIZATION_TOGGLES)
        bits[index] = "1"
        name = "perm-" + "".join(bits)
        size = sizes.get(name)
        if size is None:
            continue
        if size <= all_off:
            problems.append(
                f"{toggle}: enabling it did not grow the shader ({name} {size}B vs "
                f"perm-0000 {all_off}B) - the -D is not reaching OPENPBR_GET_SPECIALIZATION_CONSTANT"
            )
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dxc", help="path to dxc.exe (overrides the search list)")
    parser.add_argument("-k", "--filter", help="only run cases whose name contains this substring")
    parser.add_argument("--list", action="store_true", help="print the matrix and exit")
    parser.add_argument("--quick", action="store_true", help="one DXIL case per shader at its highest model")
    parser.add_argument("--werror", action="store_true", help="treat DXC warnings as failures")
    parser.add_argument("--keep-output", action="store_true", help="keep compiled output under tests/hlsl/_out")
    parser.add_argument("--show-warnings", action="store_true", help="print warnings for passing cases too")
    parser.add_argument(
        "--require-dxc-version", action="store_true", help=f"fail unless DXC is the pinned build ({PINNED_DXC_BUILD})"
    )
    parser.add_argument("-j", "--jobs", type=int, default=min(8, (os.cpu_count() or 4)), help="parallel dxc invocations")
    args = parser.parse_args()

    color = supports_color()

    def paint(text: str, code: str) -> str:
        return f"{code}{text}{RESET}" if color else text

    cases = build_matrix(quick=args.quick)
    if args.filter:
        cases = [c for c in cases if args.filter in c.name]

    if args.list:
        for case in cases:
            print(f"{case.name:<42} {case.shader.note}")
        print(f"\n{len(cases)} cases")
        return 0

    if not cases:
        print("no cases selected")
        return 1

    dxc = find_dxc(args.dxc)
    print(f"dxc      : {dxc.path}")
    print(f"version  : {dxc.version}   {paint(f'({dxc.raw_version})', DIM) if color else ''}")
    if dxc.version == PINNED_DXC_BUILD:
        print(f"pinned   : yes ({PINNED_DXC_RELEASE})")
    else:
        message = (
            f"warning  : this matrix is maintained against DXC {PINNED_DXC_RELEASE} "
            f"(build {PINNED_DXC_BUILD}), found build {dxc.version}"
        )
        if args.require_dxc_version:
            print(paint(message.replace("warning", "error  "), RED))
            print(f"           Download it from {DXC_RELEASE_URL}")
            print(f"           and unpack to {REPO_ROOT / 'tools' / 'dxc'}.")
            return 2
        print(paint(message, YELLOW))
    out_dir: Path | None = None
    if args.keep_output:
        out_dir = SHADER_DIR / "_out"
        out_dir.mkdir(parents=True, exist_ok=True)

    print(f"cases    : {len(cases)}  (jobs: {args.jobs})\n")

    results: list[Result] = []
    started = time.perf_counter()
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(run_case, case, dxc.path, out_dir, args.werror): case for case in cases}
        for future in concurrent.futures.as_completed(futures):
            result = future.result()
            results.append(result)

            status = paint("PASS", GREEN) if result.ok else paint("FAIL", RED)
            size = f"{result.output_size / 1024:.0f} KiB" if result.output_size else "-"
            warn = f"  {paint(f'{len(result.warnings)}w', YELLOW)}" if result.warnings else ""
            print(f"  {status}  {result.case.name:<42} {result.duration:5.1f}s  {size:>9}{warn}")

    total_time = time.perf_counter() - started
    results.sort(key=lambda r: r.case.name)

    failures = [r for r in results if not r.ok]

    if args.show_warnings:
        warned = [r for r in results if r.warnings and r.ok]
        if warned:
            print(f"\n{paint('warnings', YELLOW)}")
            seen: set[str] = set()
            for result in warned:
                for warning in result.warnings:
                    if warning in seen:
                        continue
                    seen.add(warning)
                    print(f"  {warning}")

    if failures:
        print(f"\n{paint('failures', RED)}")
        for result in failures:
            print(f"\n  {paint(result.case.name, RED)}  (exit {result.returncode})")
            print(f"    {DIM if color else ''}{' '.join(result.case.command(dxc.path, None))}{RESET if color else ''}")
            if result.errors:
                for error in result.errors[:10]:
                    print(f"    {error}")
                if len(result.errors) > 10:
                    print(f"    ... and {len(result.errors) - 10} more")
            elif result.warnings and args.werror:
                for warning in result.warnings[:10]:
                    print(f"    (warning, --werror) {warning}")
            else:
                print(f"    {result.raw_output.strip()[:800]}")

    toggle_problems = check_specialization_toggles(results)
    if toggle_problems:
        print(f"\n{paint('specialization toggle check failed', RED)}")
        for problem in toggle_problems:
            print(f"  {problem}")

    passed = len(results) - len(failures)
    summary = f"\n{passed}/{len(results)} passed in {total_time:.1f}s"

    perm_sizes = {r.case.variant.name: r.output_size for r in results if r.case.shader.name == "spec_constants" and r.ok}
    if not toggle_problems and "perm-0000" in perm_sizes and "perm-1111" in perm_sizes:
        summary += "  (toggles live: perm-0000 %d KiB -> perm-1111 %d KiB)" % (
            perm_sizes["perm-0000"] // 1024,
            perm_sizes["perm-1111"] // 1024,
        )

    ok = not failures and not toggle_problems
    print(paint(summary, GREEN if ok else RED))
    if out_dir:
        print(f"output kept in {out_dir}")

    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
