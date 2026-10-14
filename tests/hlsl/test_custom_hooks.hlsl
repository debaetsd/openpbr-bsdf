// Exercises the renderer-facing override hooks documented in openpbr_settings.h,
// all at once: fast-math intrinsics, a host-provided saturate(), host-provided
// vector type names, real specialization constants, and redirected assertions.
//
// This is the "integrated into an existing renderer" permutation - the one most
// likely to break, because every one of these hooks has to be visible to the
// OpenPBR headers before they are included, and the vec type override in
// particular has to supply everything the HLSL interop header would otherwise
// have defined itself.

#define OPENPBR_LANGUAGE_TARGET_HLSL 1

// --- Host-provided vector types -------------------------------------------
// A renderer that already has GLSL-style names sets this to keep OpenPBR from
// defining its own. It then owns both halves of the contract: the type names
// and the GLSL splat-capable constructors.
#define OPENPBR_USE_CUSTOM_VEC_TYPES 1

typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

#define HOST_VEC2_1(x) ((float2)(x))
#define HOST_VEC2_2(x, y) float2(x, y)
#define HOST_VEC3_1(x) ((float3)(x))
#define HOST_VEC3_2(x, y) float3(x, y)
#define HOST_VEC3_3(x, y, z) float3(x, y, z)
#define HOST_VEC4_1(x) ((float4)(x))
#define HOST_VEC4_2(x, y) float4(x, y)
#define HOST_VEC4_3(x, y, z) float4(x, y, z)
#define HOST_VEC4_4(x, y, z, w) float4(x, y, z, w)

#define HOST_CAT_IMPL(a, b) a##b
#define HOST_CAT(a, b) HOST_CAT_IMPL(a, b)
#define HOST_COUNT_IMPL(_1, _2, _3, _4, N, ...) N
#define HOST_COUNT(...) HOST_COUNT_IMPL(__VA_ARGS__, 4, 3, 2, 1)

#define vec2(...) HOST_CAT(HOST_VEC2_, HOST_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define vec3(...) HOST_CAT(HOST_VEC3_, HOST_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define vec4(...) HOST_CAT(HOST_VEC4_, HOST_COUNT(__VA_ARGS__))(__VA_ARGS__)

// --- Host-provided saturate() ---------------------------------------------
// HLSL has saturate() built in, so OPENPBR_USE_CUSTOM_SATURATE is a no-op for
// this backend. Setting it anyway confirms it does not break the build.
#define OPENPBR_USE_CUSTOM_SATURATE 1

// --- Fast-math hooks ------------------------------------------------------
#define OPENPBR_FAST_RCP_SQRT(x) rsqrt(x)
#define OPENPBR_FAST_SQRT(x) sqrt(x)
#define OPENPBR_FAST_NORMALIZE(v) normalize(v)

// --- Specialization constants ---------------------------------------------
// A real renderer routes these through root constants, a constant buffer, or
// SPIR-V [[vk::constant_id]]. A constant buffer is the portable choice and,
// unlike a literal true, it keeps DXC from folding the disabled branches away -
// so both sides of every toggle get compiled.
cbuffer OpenPBR_FeatureToggles : register(b0)
{
    bool g_EnableSheenAndCoat;
    bool g_EnableDispersion;
    bool g_EnableTranslucency;
    bool g_EnableMetallic;
};

#define OPENPBR_GET_SPECIALIZATION_CONSTANT(name) g_##name

// --- Assertion hooks ------------------------------------------------------
// Redirected to nothing, but through a distinct macro so a stray OPENPBR_ASSERT
// expansion would still show up as a compile error rather than vanishing.
#define OPENPBR_HOST_ASSERT(expr) \
    do                            \
    {                             \
    } while (false)
#define OPENPBR_ASSERT(expr, message) OPENPBR_HOST_ASSERT(expr)
#define OPENPBR_ASSERT_UNREACHABLE(message) OPENPBR_HOST_ASSERT(false)
#define OPENPBR_STATIC_ASSERT(expr, message)

// --- Appearance setting ---------------------------------------------------
#define OPENPBR_RECIPROCAL_COAT_AND_FUZZ 1

#include "openpbr.h"

#include "openpbr_test_workload.hlsli"

RWStructuredBuffer<float4> Output : register(u0);

[numthreads(64, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
    Output[thread_id.x] = openpbr_test_workload(float(thread_id.x));
}
