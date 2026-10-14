// DXR library target. This is the primary integration path for a path tracer,
// and lib_6_* compiles under different rules than a standalone entry point
// (exported functions, no single entry, payload-carrying stages).

#define OPENPBR_LANGUAGE_TARGET_HLSL 1
#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

// Payload access qualifiers are honored from lib_6_7 on (lib_6_6 needs
// -enable-payload-qualifiers and warns otherwise) and are mandatory at lib_6_8,
// so the attribute has to be conditional for this one file to cover the whole
// lib_6_3 .. lib_6_8 range. DXC defines __SHADER_TARGET_MAJOR/MINOR from the
// -T profile.
#if __SHADER_TARGET_MAJOR > 6 || (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 7)
#define OPENPBR_TEST_RAYPAYLOAD [raypayload]
#define OPENPBR_TEST_PAYLOAD_QUALIFIERS : read(caller) : write(closesthit)
#else
#define OPENPBR_TEST_RAYPAYLOAD
#define OPENPBR_TEST_PAYLOAD_QUALIFIERS
#endif

struct OPENPBR_TEST_RAYPAYLOAD Payload
{
    float4 color OPENPBR_TEST_PAYLOAD_QUALIFIERS;
};

[shader("closesthit")]
void ClosestHit(inout Payload payload, in BuiltInTriangleIntersectionAttributes attributes)
{
    payload.color = openpbr_test_workload(attributes.barycentrics.x);
}

[shader("raygeneration")]
void RayGen()
{
    // No trace here - this test only needs the OpenPBR code to compile into a
    // library, not a runnable pipeline.
}
