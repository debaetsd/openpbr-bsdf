// Baseline: the default OpenPBR configuration in a compute shader.
// This is the permutation most consumers will use.

#define OPENPBR_LANGUAGE_TARGET_HLSL 1
#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

RWStructuredBuffer<float4> Output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
    Output[thread_id.x] = openpbr_test_workload(float(thread_id.x));
}
