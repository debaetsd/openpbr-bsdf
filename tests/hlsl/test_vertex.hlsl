// The vertex-shader stage. OpenPBR in a vertex shader is unusual, but this
// catches anything that quietly depends on pixel-shader-only facilities
// (implicit derivatives, discard, and so on).

#define OPENPBR_LANGUAGE_TARGET_HLSL 1
#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

float4 main(uint vertex_id : SV_VertexID) : SV_Position
{
    return openpbr_test_workload(float(vertex_id));
}
