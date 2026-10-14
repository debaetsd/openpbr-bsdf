/***************************************************************************
 * Copyright 2026 Adobe
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ***************************************************************************/

// OpenPBR HLSL interop macros.
//
// This header provides HLSL-specific macro definitions so OpenPBR code can
// compile with DXC (DirectX Shader Compiler) targeting Shader Model 6.x and
// DXIL, for D3D12.
//
// Requirements:
//   - DXC (dxc.exe / dxcompiler). FXC (Shader Model 5.x) is not supported.
//   - HLSL 2021 or newer. 2021 is the DXC default from the April 2022 release
//     onward; the check below fails loudly on older language modes rather than
//     letting them miscompile. tests/compile_hlsl.py builds with -HV 202x, so
//     both 2021 and 202x are covered.
//   - Shader Model 6.0 or newer.
//
// HLSL differs from the GLSL-flavored syntax OpenPBR is written in, and from
// Slang (which is HLSL-like but has its own extensions), in four ways that
// matter here. Each is handled below:
//
//   1. Vector splats.  GLSL's vec3(x) broadcasts a scalar; HLSL's float3(x) is
//      an error ("too few elements in vector initialization"). vec2/vec3/vec4
//      are therefore both typedefs (for declarations) and arity-dispatching
//      function-like macros (for construction).
//   2. Struct construction.  HLSL has no constructor syntax for user-defined
//      structs, and braced initializers are only valid in a declaration, not in
//      expression position. OPENPBR_MAKE_STRUCT_N therefore expands to
//      type::ctor(...), a static member function each struct declares under
//      OPENPBR_LANGUAGE_TARGET_HLSL.
//   3. GLSL-named built-ins.  mix/equal/notEqual/greaterThan/greaterThanEqual
//      are aliased to their HLSL equivalents (as in the Slang backend).
//   4. static_assert.  HLSL spells it _Static_assert.
//
// See openpbr_settings.h for the user-facing configuration surface.

#ifndef OPENPBR_INTEROP_HLSL_H
#define OPENPBR_INTEROP_HLSL_H

// ---------------------------------------------------------------------------
// Compiler / language-version validation
// ---------------------------------------------------------------------------

#if !defined(__hlsl_dx_compiler)
#error "Only tested on DXC"
#endif

#if defined(__HLSL_VERSION) && __HLSL_VERSION < 2021
#error "Only tested on HLSL 2021 or newer. Pass -HV 2021 or -HV 202x to DXC."
#endif

// Memory qualifiers / address space specifiers.
// HLSL does not use function-parameter address space qualifiers.
#define OPENPBR_ADDRESS_SPACE_THREAD

// Parameter passing macros.
// HLSL-style out/inout parameter qualifiers.
#define OPENPBR_OUT(type) out type
#define OPENPBR_INOUT(type) inout type
#define OPENPBR_CONST_REF(type) const type

// Constant declaration macros.
// HLSL uses const for local constants and static const for global constants.
#define OPENPBR_CONSTEXPR_LOCAL const
#define OPENPBR_CONSTEXPR_GLOBAL static const

// Constexpr function qualifiers.
// HLSL does not have a constexpr concept for functions.
#define OPENPBR_CONSTEXPR_FUNCTION

// Vector-typed qualifiers. HLSL uses const (local) / static const (global); no
// constexpr concept. #ifndef-guarded so an OPENPBR_USE_CUSTOM_VEC_TYPES host
// whose vector types differ can predefine them.
#ifndef OPENPBR_MAYBE_CONSTEXPR_LOCAL
#define OPENPBR_MAYBE_CONSTEXPR_LOCAL const
#endif
#ifndef OPENPBR_MAYBE_CONSTEXPR_GLOBAL
#define OPENPBR_MAYBE_CONSTEXPR_GLOBAL static const
#endif
#ifndef OPENPBR_MAYBE_CONSTEXPR_FUNCTION
#define OPENPBR_MAYBE_CONSTEXPR_FUNCTION
#endif

// HLSL does not require an explicit inline keyword.
#define OPENPBR_INLINE_FUNCTION

// ---------------------------------------------------------------------------
// Backend-active flag
// ---------------------------------------------------------------------------

// Force the flag to 1 now that this backend is definitely the active one.
//
// The selector in openpbr_interop.h reaches this header by either route: an
// explicit OPENPBR_LANGUAGE_TARGET_HLSL = 1, or auto-detection from
// __hlsl_dx_compiler - and in the auto-detected case the flag is still 0. The
// per-struct ctor() members are guarded on this flag, so leaving it 0 would
// silently omit them and every OPENPBR_MAKE_STRUCT_N call site would fail to
// resolve. Setting it here also lets renderer code test
// OPENPBR_LANGUAGE_TARGET_HLSL and get a truthful answer.
//
// This runs after the selector has finished validating the flags, so
// overwriting it cannot affect that validation.
#undef OPENPBR_LANGUAGE_TARGET_HLSL
#define OPENPBR_LANGUAGE_TARGET_HLSL 1

// ---------------------------------------------------------------------------
// Preprocessor utilities (arity dispatch)
// ---------------------------------------------------------------------------

// Two-step concatenation so macro arguments are expanded before pasting.
#define OPENPBR_HLSL_CAT_IMPL(a, b) a##b
#define OPENPBR_HLSL_CAT(a, b) OPENPBR_HLSL_CAT_IMPL(a, b)

// Counts 1..4 macro arguments. Used to pick the right vector constructor.
#define OPENPBR_HLSL_COUNT_IMPL(_1, _2, _3, _4, N, ...) N
#define OPENPBR_HLSL_COUNT(...) OPENPBR_HLSL_COUNT_IMPL(__VA_ARGS__, 4, 3, 2, 1)

// ---------------------------------------------------------------------------
// Vector types
// ---------------------------------------------------------------------------
//
// OpenPBR code uses GLSL's vec2/vec3/vec4 names in two distinct roles:
//
//   as a type        vec3 v;   OPENPBR_OUT(vec3)   struct { vec3 n; }
//   as a constructor vec3(0.0f)   vec3(x, y, z)
//
// The typedefs cover the first role. The function-like macros cover the second,
// and only expand when the name is immediately followed by '(' - so
// declarations are untouched. Arity dispatch is what makes the GLSL splat form
// work: HLSL rejects float3(x) but accepts the equivalent cast (float3)(x),
// which also evaluates x exactly once.
//
// Set OPENPBR_USE_CUSTOM_VEC_TYPES = 1 to suppress all of this if your host
// already provides GLSL-compatible vector names.
#ifndef OPENPBR_USE_CUSTOM_VEC_TYPES
#define OPENPBR_USE_CUSTOM_VEC_TYPES 0
#endif
#if !OPENPBR_USE_CUSTOM_VEC_TYPES

typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;

// clang-format off
#define OPENPBR_HLSL_VEC2_1(x)          ((float2)(x))
#define OPENPBR_HLSL_VEC2_2(x, y)       float2(x, y)

#define OPENPBR_HLSL_VEC3_1(x)          ((float3)(x))
#define OPENPBR_HLSL_VEC3_2(x, y)       float3(x, y)
#define OPENPBR_HLSL_VEC3_3(x, y, z)    float3(x, y, z)

#define OPENPBR_HLSL_VEC4_1(x)          ((float4)(x))
#define OPENPBR_HLSL_VEC4_2(x, y)       float4(x, y)
#define OPENPBR_HLSL_VEC4_3(x, y, z)    float4(x, y, z)
#define OPENPBR_HLSL_VEC4_4(x, y, z, w) float4(x, y, z, w)
// clang-format on

#define vec2(...) OPENPBR_HLSL_CAT(OPENPBR_HLSL_VEC2_, OPENPBR_HLSL_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define vec3(...) OPENPBR_HLSL_CAT(OPENPBR_HLSL_VEC3_, OPENPBR_HLSL_COUNT(__VA_ARGS__))(__VA_ARGS__)
#define vec4(...) OPENPBR_HLSL_CAT(OPENPBR_HLSL_VEC4_, OPENPBR_HLSL_COUNT(__VA_ARGS__))(__VA_ARGS__)

#endif  // !OPENPBR_USE_CUSTOM_VEC_TYPES

// ---------------------------------------------------------------------------
// Integer types
// ---------------------------------------------------------------------------

// uint is a built-in 32-bit type in HLSL.
#ifndef OPENPBR_UINT32
#define OPENPBR_UINT32 uint
#endif

// uint16_t exists only when DXC is invoked with -enable-16bit-types
// (Shader Model 6.2+), which also defines __HLSL_ENABLE_16_BIT.
#ifndef OPENPBR_UINT16
#define OPENPBR_UINT16 uint16_t
#endif

// Energy table storage: use native 16-bit integers when they are available
// (halving the table size), otherwise fall back to uint.
#ifndef OPENPBR_ENERGY_TABLES_USE_UINT16
#if defined(__HLSL_ENABLE_16_BIT)
#define OPENPBR_ENERGY_TABLES_USE_UINT16 1
#else
#define OPENPBR_ENERGY_TABLES_USE_UINT16 0
#endif
#endif

// ---------------------------------------------------------------------------
// GLSL-style naming compatibility
// ---------------------------------------------------------------------------
//
// OpenPBR code is written in GLSL-style syntax (mix/equal/notEqual/greaterThan/
// ...) while HLSL uses different names or relies on operators directly. These
// aliases let OpenPBR compile without forcing users to provide extra wrapper
// macros.
//
// Note on HLSL 2021: the comparison operators below return a bool vector for
// vector operands, matching GLSL. OpenPBR always feeds those through
// any()/all(), which are HLSL built-ins, so no select()/and()/or() shims are
// needed.
#ifndef mix
#define mix lerp
#endif
#ifndef equal
#define equal(a, b) ((a) == (b))
#endif
#ifndef notEqual
#define notEqual(a, b) ((a) != (b))
#endif
#ifndef greaterThan
#define greaterThan(a, b) ((a) > (b))
#endif
#ifndef greaterThanEqual
#define greaterThanEqual(a, b) ((a) >= (b))
#endif

// Swizzle helper.
#define OPENPBR_SWIZZLE(v, suffix) (v).suffix

// Struct constructor declaration.
//
// Placed inside a struct body, this declares whatever construction support the
// target language needs. HLSL is the only backend that needs anything: it emits
// a static ctor() member for OPENPBR_MAKE_STRUCT_N to call. Every other backend
// defines these to nothing and keeps using its own construction syntax, which is
// why the arguments are (type, then a type/name pair per field) rather than
// anything HLSL-shaped.
//
//     struct OpenPBR_Basis
//     {
//         vec3 t, b, n;
//
//         OPENPBR_STRUCT_CTOR_3(OpenPBR_Basis, vec3, t, vec3, b, vec3, n)
//     };
//
// The arity suffix is explicit, and the macros are not variadic, because the
// GLSL preprocessor supports neither __VA_ARGS__ nor overloading on argument
// count - the same reason OPENPBR_MAKE_STRUCT_N below is spelled this way.
//
// Fields whose type cannot be written as a plain `type name` pair - arrays, in
// particular - cannot go through these macros; such structs declare ctor() by
// hand under #if OPENPBR_LANGUAGE_TARGET_HLSL.
// clang-format off
#define OPENPBR_STRUCT_CTOR_1(type, t1, n1) static type ctor(const t1 n1) { type result; result.n1 = n1; return result; }
#define OPENPBR_STRUCT_CTOR_2(type, t1, n1, t2, n2) static type ctor(const t1 n1, const t2 n2) { type result; result.n1 = n1; result.n2 = n2; return result; }
#define OPENPBR_STRUCT_CTOR_3(type, t1, n1, t2, n2, t3, n3) static type ctor(const t1 n1, const t2 n2, const t3 n3) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; return result; }
#define OPENPBR_STRUCT_CTOR_4(type, t1, n1, t2, n2, t3, n3, t4, n4) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; return result; }
#define OPENPBR_STRUCT_CTOR_5(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; return result; }
#define OPENPBR_STRUCT_CTOR_6(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; return result; }
#define OPENPBR_STRUCT_CTOR_7(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; return result; }
#define OPENPBR_STRUCT_CTOR_8(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; return result; }
#define OPENPBR_STRUCT_CTOR_9(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; return result; }
#define OPENPBR_STRUCT_CTOR_10(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9, const t10 n10) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; result.n10 = n10; return result; }
#define OPENPBR_STRUCT_CTOR_11(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9, const t10 n10, const t11 n11) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; result.n10 = n10; result.n11 = n11; return result; }
#define OPENPBR_STRUCT_CTOR_12(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9, const t10 n10, const t11 n11, const t12 n12) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; result.n10 = n10; result.n11 = n11; result.n12 = n12; return result; }
#define OPENPBR_STRUCT_CTOR_13(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9, const t10 n10, const t11 n11, const t12 n12, const t13 n13) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; result.n10 = n10; result.n11 = n11; result.n12 = n12; result.n13 = n13; return result; }
#define OPENPBR_STRUCT_CTOR_14(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9, const t10 n10, const t11 n11, const t12 n12, const t13 n13, const t14 n14) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; result.n10 = n10; result.n11 = n11; result.n12 = n12; result.n13 = n13; result.n14 = n14; return result; }
#define OPENPBR_STRUCT_CTOR_15(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14, t15, n15) static type ctor(const t1 n1, const t2 n2, const t3 n3, const t4 n4, const t5 n5, const t6 n6, const t7 n7, const t8 n8, const t9 n9, const t10 n10, const t11 n11, const t12 n12, const t13 n13, const t14 n14, const t15 n15) { type result; result.n1 = n1; result.n2 = n2; result.n3 = n3; result.n4 = n4; result.n5 = n5; result.n6 = n6; result.n7 = n7; result.n8 = n8; result.n9 = n9; result.n10 = n10; result.n11 = n11; result.n12 = n12; result.n13 = n13; result.n14 = n14; result.n15 = n15; return result; }
// clang-format on

// Struct-construction helpers.
//
// HLSL has no constructor syntax for user-defined structs, and a braced
// initializer list is only valid in a declaration - both `s = {a, b};` and
// `return {a, b};` are parse errors. So unlike every other backend, these
// macros cannot expand to `type(args...)`.
//
// Instead every OpenPBR struct built this way declares a static member function
// named ctor(), guarded by OPENPBR_LANGUAGE_TARGET_HLSL, and the macros call
// it. Naming the member rather than pasting a free function name keeps `type`
// an ordinary macro argument, so the generic lobe headers can keep passing
// OPENPBR_MINIMAL_MICROFACET_LOBE_TYPE and have it expand normally.
//
// Adding a struct that is built through OPENPBR_MAKE_STRUCT_N therefore also
// means adding its ctor().
// clang-format off
#define OPENPBR_MAKE_STRUCT_1(type, arg1) type::ctor(arg1)
#define OPENPBR_MAKE_STRUCT_2(type, arg1, arg2) type::ctor(arg1, arg2)
#define OPENPBR_MAKE_STRUCT_3(type, arg1, arg2, arg3) type::ctor(arg1, arg2, arg3)
#define OPENPBR_MAKE_STRUCT_4(type, arg1, arg2, arg3, arg4) type::ctor(arg1, arg2, arg3, arg4)
#define OPENPBR_MAKE_STRUCT_5(type, arg1, arg2, arg3, arg4, arg5) type::ctor(arg1, arg2, arg3, arg4, arg5)
#define OPENPBR_MAKE_STRUCT_6(type, arg1, arg2, arg3, arg4, arg5, arg6) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6)
#define OPENPBR_MAKE_STRUCT_7(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define OPENPBR_MAKE_STRUCT_8(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define OPENPBR_MAKE_STRUCT_9(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)
#define OPENPBR_MAKE_STRUCT_10(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10)
#define OPENPBR_MAKE_STRUCT_11(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11)
#define OPENPBR_MAKE_STRUCT_12(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12)
#define OPENPBR_MAKE_STRUCT_13(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13)
#define OPENPBR_MAKE_STRUCT_14(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14)
#define OPENPBR_MAKE_STRUCT_15(type, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15) type::ctor(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14, arg15)
// clang-format on

// saturate() relies on the HLSL language built-in, which is overloaded for
// scalars and vectors; no shim is needed.
// Note: NaN behavior may differ from the explicit ternary shim used in the C++
// and GLSL backends. CUDA, MSL, and Slang also rely on their respective
// language built-ins with the same caveat.

// ---------------------------------------------------------------------------
// Assertions
// ---------------------------------------------------------------------------

// Override any of these by pre-defining before including openpbr.h.
// HLSL has no portable runtime assert mechanism, so those are silent no-ops.
// Compile-time assertions are real: DXC spells static_assert as _Static_assert.
// do-while(false) makes each macro a single statement, safe in if/else without
// braces.
#ifndef OPENPBR_ASSERT
#define OPENPBR_ASSERT(expr, message)                                                                                                                \
    do                                                                                                                                               \
    {                                                                                                                                                \
    } while (false)
#endif
#ifndef OPENPBR_ASSERT_UNREACHABLE
#define OPENPBR_ASSERT_UNREACHABLE(message)                                                                                                          \
    do                                                                                                                                               \
    {                                                                                                                                                \
    } while (false)
#endif
#ifndef OPENPBR_STATIC_ASSERT
#define OPENPBR_STATIC_ASSERT(expr, message) _Static_assert(expr, message)
#endif

// ---------------------------------------------------------------------------
// Specialization constants
// ---------------------------------------------------------------------------

// Default specialization-constant hook. Override before including any OpenPBR
// header if your renderer provides its own specialization constant pipeline.
// D3D12 has no specialization constants, so this is typically a -D define
// (compiling one shader permutation per toggle combination) or a root constant
// read at runtime.
#ifndef OPENPBR_GET_SPECIALIZATION_CONSTANT
#define OPENPBR_GET_SPECIALIZATION_CONSTANT(name) true
#endif

#endif  // OPENPBR_INTEROP_HLSL_H
