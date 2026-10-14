// No OPENPBR_LANGUAGE_TARGET_* is set here on purpose: this verifies that the
// selector in interop/openpbr_interop.h picks the HLSL backend on its own from
// DXC's __hlsl_dx_compiler define. If auto-detection regressed, this would fall
// through to the GLSL fallback and fail to compile.

#include "openpbr.h"

#if !OPENPBR_LANGUAGE_TARGET_HLSL && !defined(OPENPBR_INTEROP_HLSL_H)
#error "Auto-detection did not select the HLSL interop backend under DXC."
#endif

#include "openpbr_test_workload.hlsli"

RWStructuredBuffer<float4> Output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
    Output[thread_id.x] = openpbr_test_workload(float(thread_id.x));
}
