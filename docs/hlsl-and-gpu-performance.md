# HLSL backend and GPU code size

Changes in this fork relative to upstream `c91aad1`.

## HLSL / DXC backend

`interop/openpbr_interop_hlsl.h` targets DXC (HLSL 2021+, SM 6.x, DXIL), selected
from `__hlsl_dx_compiler`.

- **Vector construction**: `vec2/3/4` are typedefs plus arity-dispatching macros,
  so GLSL's scalar splat `vec3(x)` becomes `(float3)(x)`.
- **Struct construction**: HLSL has no constructors and no braced initializers
  outside declarations, so `OPENPBR_MAKE_STRUCT_N` expands to `type::ctor(...)`,
  declared per struct through `OPENPBR_STRUCT_CTOR_N` (empty on every other
  backend, with fallbacks in `openpbr_interop.h` for custom interop layers).
- The GLSL names the library is written in (`vec2/3/4`, `mix`, `equal`,
  `notEqual`, `greaterThan`, `greaterThanEqual`) are macros; host code using
  those names after including `openpbr.h` sees them.

## Why code size

GPU compilers inline every call, so cost = size x call sites. In a DXR path
tracer's ray generation shader (RTX 3090) the full BSDF was 133 K SASS
instructions; Nsight showed 29 % of samples stalled on instruction fetch, 57 %
on memory, 5.7 % SM issue, 62 % VRAM throughput, and register-limited
occupancy. From the DXIL: thin-film reflectance inlined 39x (33 % of
instructions), lobe value/pdf ~70x each, and 443 dynamically indexed `alloca`s
(local memory) from per-channel loops.

## Exact changes (always on)

Bit-identical to the previous commit on 60 K randomized cases (all inputs,
both sides, stochastic wavelengths, edge cases) in 7 feature-toggle
combinations.

| change | effect |
|---|---|
| `OPENPBR_UNROLL` / `OPENPBR_DONT_UNROLL` (per backend, overridable), `openpbr_component` / `openpbr_with_component` (select-based) | per-channel loops rolled without dynamic indexing: no `alloca`s |
| `openpbr_calculate_lobe_value_and_pdf` per lobe; public `openpbr_eval_and_pdf` | value + pdf in one pass, one inlined copy; sampling uses it for unpicked lobes |
| coat layer reaches its base from one call site (value, pdf, sampling) | removes duplicate copies of the base aggregate |
| specular lobe: one coefficient evaluation for value and pdf, reflection + all refraction half vectors in one loop | thin film/Fresnel once per lobe copy |
| dispersion compiled in, equal channel IORs: transmission loop runs once and broadcasts | 3x less transmission work for non-dispersive materials |

Path tracer DXR library, ray generation shader:

| | DXIL | RG instructions | OpenPBR `alloca`s | frame |
|---|---:|---:|---:|---:|
| before | 389 KB | 102 K | 443 | 145-192 ms |
| after | 185 KB | 52 K | 0 | 62-64 ms |

Inlined copies: thin film 39 -> 7, aggregate sampling 21 -> 12, lobe value/pdf
~70 -> 23. Cold pipeline compile 132 s -> 11 s.

## Opt-in options (`openpbr_settings.h`, default 0)

**`OPENPBR_SAMPLE_REEVALUATE`**: layers only choose the direction; `openpbr_sample`
evaluates the BSDF once there and returns exactly `openpbr_pdf`.
`openpbr_sample_direction` skips that evaluation for integrators that evaluate
the direction anyway (MIS). Not exact: the picked lobe is re-evaluated from the
final direction instead of its sampling formulas. Directions, types and validity
unchanged; weights > 1 % off in 0.5 % of cases (p99 0.18 %); pdfs > 1 % off in
3 %, up to tens of percent on near-specular lobes. Mean weight (= albedo)
unchanged to < 1e-5.

**`OPENPBR_SKIP_INACTIVE_LOBES`**: skips terms that provably contribute nothing -
a base lobe's value when its scale is zero, its pdf when its sampling weight is
zero, coat/sheen values at zero presence, the coat pdf at zero selection
probability. Bit-identical on the 60 K cases; only NaN/Inf propagation through
`0 * x` can differ.

| | DXIL | frame |
|---|---:|---:|
| default | 185 KB | 63 ms |
| reevaluate | 139 KB | 54 ms (-15 %) |
| skip inactive | 187 KB | within run-to-run noise |

Reevaluate renders are unchanged within run-to-run noise (75-80 dB PSNR on
layered/transmissive test materials; an emissive-only room converges to the
BSDF-sampling reference within 0.04 %).

## Tests

- `tests/compile_hlsl.py`: DXC matrix over stages (cs/ps/vs/DXR lib), SM
  6.0-6.8 and configurations (texture LUTs, 16-bit tables, hooks, -O0/-O3,
  debug info, the 16 specialization-constant permutations, both options).
- `tests/compile_glsl.py`: glslang, default and both options.
- `tests/cpp/test_consistency.cpp`: `openpbr_eval_and_pdf` == `openpbr_eval` +
  `openpbr_pdf` bitwise; `openpbr_sample_direction` == `openpbr_sample` in
  direction, type and validity; with `OPENPBR_SAMPLE_REEVALUATE`, sample pdf ==
  `openpbr_pdf`.
