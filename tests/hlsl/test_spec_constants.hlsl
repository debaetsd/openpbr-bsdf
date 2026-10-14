// OPENPBR_GET_SPECIALIZATION_CONSTANT wired up as shader compilation
// permutations: each feature toggle resolves to a literal true/false at compile
// time, so DXC dead-code-eliminates the disabled lobes entirely.
//
// D3D12 has no specialization constants, so this - one shader variant per toggle
// combination, selected with -D - is the way a D3D12 renderer actually consumes
// these toggles. It is a different code path from the runtime-uniform version in
// test_custom_hooks.hlsl: there the toggles are cbuffer reads and both sides of
// every branch get compiled, whereas here whole lobes disappear.
//
// tests/compile_hlsl.py builds all 2^4 = 16 combinations and checks that the
// all-off variant really is smaller than the all-on one, which is what proves
// the toggles are reaching the implementation rather than being quietly ignored.
//
// Each toggle defaults to on so the file is still meaningful compiled by hand.

#define OPENPBR_LANGUAGE_TARGET_HLSL 1

#ifndef OPENPBR_PERMUTATION_EnableSheenAndCoat
#define OPENPBR_PERMUTATION_EnableSheenAndCoat 1
#endif
#ifndef OPENPBR_PERMUTATION_EnableDispersion
#define OPENPBR_PERMUTATION_EnableDispersion 1
#endif
#ifndef OPENPBR_PERMUTATION_EnableTranslucency
#define OPENPBR_PERMUTATION_EnableTranslucency 1
#endif
#ifndef OPENPBR_PERMUTATION_EnableMetallic
#define OPENPBR_PERMUTATION_EnableMetallic 1
#endif

// The extra indirection is what lets `name` paste: ## suppresses expansion of
// its operands, so OPENPBR_GET_SPECIALIZATION_CONSTANT has to hand the token to
// a second macro that does the pasting.
#define OPENPBR_PERMUTATION_VALUE(name) OPENPBR_PERMUTATION_##name
#define OPENPBR_GET_SPECIALIZATION_CONSTANT(name) (OPENPBR_PERMUTATION_VALUE(name) != 0)

#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

RWStructuredBuffer<float4> Output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
    Output[thread_id.x] = openpbr_test_workload(float(thread_id.x));
}
