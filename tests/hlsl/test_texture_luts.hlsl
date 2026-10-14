// OPENPBR_USE_TEXTURE_LUTS = 1: the lookup tables come from bound textures
// instead of the built-in constant arrays. This exercises the renderer-supplied
// OPENPBR_SAMPLE_2D_TEXTURE / OPENPBR_SAMPLE_3D_TEXTURE hooks and, just as
// importantly, checks that the large constant arrays really do drop out.
//
// The bindless array layout below mirrors the OpenPBR_LutId_* constants in
// openpbr_data_constants.h, which are consecutive integers in [0, 8).

#define OPENPBR_LANGUAGE_TARGET_HLSL 1
#define OPENPBR_USE_TEXTURE_LUTS 1

Texture2D<float4> OpenPBR_Luts2D[8] : register(t0, space1);
Texture3D<float4> OpenPBR_Luts3D[8] : register(t0, space2);
SamplerState OpenPBR_LutSampler : register(s0);

#define OPENPBR_SAMPLE_2D_TEXTURE(lut_id, uv) OpenPBR_Luts2D[lut_id].SampleLevel(OpenPBR_LutSampler, uv, 0.0f)
#define OPENPBR_SAMPLE_3D_TEXTURE(lut_id, uvw) OpenPBR_Luts3D[lut_id].SampleLevel(OpenPBR_LutSampler, uvw, 0.0f)

#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

RWStructuredBuffer<float4> Output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
    Output[thread_id.x] = openpbr_test_workload(float(thread_id.x));
}
