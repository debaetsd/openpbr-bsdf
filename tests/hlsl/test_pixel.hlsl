// The pixel-shader stage. Worth covering separately from compute because DXC
// applies different validation rules per stage (and because a raster renderer
// shading a G-buffer is a realistic OpenPBR integration).

#define OPENPBR_LANGUAGE_TARGET_HLSL 1
#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

float4 main(float4 position : SV_Position) : SV_Target
{
    return openpbr_test_workload(position.x);
}
