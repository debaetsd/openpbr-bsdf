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

// OpenPBR cross-language interop macro selector.
//
// This header automatically detects the target shading language and includes
// the appropriate language-specific interop header. It can be overridden by
// explicitly setting OPENPBR_LANGUAGE_TARGET_* defines in openpbr_settings.h
// or before including OpenPBR headers.
//
// When OPENPBR_USE_CUSTOM_INTEROP = 1, this file does nothing and the user
// must provide their own interop macros before including OpenPBR headers.

#ifndef OPENPBR_INTEROP_H
#define OPENPBR_INTEROP_H

#ifndef OPENPBR_USE_CUSTOM_INTEROP
#define OPENPBR_USE_CUSTOM_INTEROP 0
#endif

#if !OPENPBR_USE_CUSTOM_INTEROP

// Ensure target flags exist even if openpbr_settings.h was not included first.
#ifndef OPENPBR_LANGUAGE_TARGET_CPP
#define OPENPBR_LANGUAGE_TARGET_CPP 0
#endif
#ifndef OPENPBR_LANGUAGE_TARGET_CUDA
#define OPENPBR_LANGUAGE_TARGET_CUDA 0
#endif
#ifndef OPENPBR_LANGUAGE_TARGET_GLSL
#define OPENPBR_LANGUAGE_TARGET_GLSL 0
#endif
#ifndef OPENPBR_LANGUAGE_TARGET_HLSL
#define OPENPBR_LANGUAGE_TARGET_HLSL 0
#endif
#ifndef OPENPBR_LANGUAGE_TARGET_MSL
#define OPENPBR_LANGUAGE_TARGET_MSL 0
#endif
#ifndef OPENPBR_LANGUAGE_TARGET_SLANG
#define OPENPBR_LANGUAGE_TARGET_SLANG 0
#endif

// Validate explicit language-target flags.
// Allowed states:
//   - exactly one flag is 1 (explicit backend override)
//   - all flags are 0 (auto-detect backend)
// Any other combination is ambiguous and rejected.

#if (OPENPBR_LANGUAGE_TARGET_CPP != 0 && OPENPBR_LANGUAGE_TARGET_CPP != 1) ||                                                                        \
    (OPENPBR_LANGUAGE_TARGET_CUDA != 0 && OPENPBR_LANGUAGE_TARGET_CUDA != 1) ||                                                                      \
    (OPENPBR_LANGUAGE_TARGET_GLSL != 0 && OPENPBR_LANGUAGE_TARGET_GLSL != 1) ||                                                                      \
    (OPENPBR_LANGUAGE_TARGET_HLSL != 0 && OPENPBR_LANGUAGE_TARGET_HLSL != 1) ||                                                                      \
    (OPENPBR_LANGUAGE_TARGET_MSL != 0 && OPENPBR_LANGUAGE_TARGET_MSL != 1) ||                                                                        \
    (OPENPBR_LANGUAGE_TARGET_SLANG != 0 && OPENPBR_LANGUAGE_TARGET_SLANG != 1)
#error "OPENPBR_LANGUAGE_TARGET_* flags must each be 0 or 1."
#endif

#if (OPENPBR_LANGUAGE_TARGET_CPP + OPENPBR_LANGUAGE_TARGET_CUDA + OPENPBR_LANGUAGE_TARGET_GLSL +                                                     \
     OPENPBR_LANGUAGE_TARGET_HLSL + OPENPBR_LANGUAGE_TARGET_MSL + OPENPBR_LANGUAGE_TARGET_SLANG) > 1
#error "Ambiguous OpenPBR target configuration: set at most one OPENPBR_LANGUAGE_TARGET_* flag to 1 (or leave all 0 for auto-detect)."
#endif

// Language detection and selection logic.
// Priority order:
//   1. Explicit OPENPBR_LANGUAGE_TARGET_* setting from openpbr_settings.h
//   2. Compiler-specific macro detection
//   3. GLSL as fallback default

#if OPENPBR_LANGUAGE_TARGET_CPP
#include "openpbr_interop_cpp.h"
#elif OPENPBR_LANGUAGE_TARGET_CUDA
#include "openpbr_interop_cuda.h"
#elif OPENPBR_LANGUAGE_TARGET_GLSL
#include "openpbr_interop_glsl.h"
#elif OPENPBR_LANGUAGE_TARGET_HLSL
#include "openpbr_interop_hlsl.h"
#elif OPENPBR_LANGUAGE_TARGET_MSL
#include "openpbr_interop_msl.h"
#elif OPENPBR_LANGUAGE_TARGET_SLANG
#include "openpbr_interop_slang.h"
#else
// No explicit target set - auto-detect based on compiler defines.
// Check most specific defines first to avoid misidentification.
#if defined(__CUDACC__)
// CUDA compiler (also defines __cplusplus, so check this first)
#include "openpbr_interop_cuda.h"
#elif defined(__METAL_VERSION__)
#include "openpbr_interop_msl.h"
#elif defined(__hlsl_dx_compiler)
#include "openpbr_interop_hlsl.h"
#elif defined(__cplusplus)
// C++ compiler (checked last since CUDA also defines this)
#include "openpbr_interop_cpp.h"
#else
// Default to GLSL for generic shader compilers.
#include "openpbr_interop_glsl.h"
#endif
#endif

#endif  // !OPENPBR_USE_CUSTOM_INTEROP

// Hooks added after a custom interop layer may have been written: give them a
// neutral default here so such a layer keeps compiling without defining them.
// OPENPBR_STRUCT_CTOR_N declares HLSL's per-struct constructor and is empty on
// every other backend; non-variadic, as the GLSL preprocessor requires.
#ifndef OPENPBR_STRUCT_CTOR_1
#define OPENPBR_STRUCT_CTOR_1(type, t1, n1)
#endif
#ifndef OPENPBR_STRUCT_CTOR_2
#define OPENPBR_STRUCT_CTOR_2(type, t1, n1, t2, n2)
#endif
#ifndef OPENPBR_STRUCT_CTOR_3
#define OPENPBR_STRUCT_CTOR_3(type, t1, n1, t2, n2, t3, n3)
#endif
#ifndef OPENPBR_STRUCT_CTOR_4
#define OPENPBR_STRUCT_CTOR_4(type, t1, n1, t2, n2, t3, n3, t4, n4)
#endif
#ifndef OPENPBR_STRUCT_CTOR_5
#define OPENPBR_STRUCT_CTOR_5(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5)
#endif
#ifndef OPENPBR_STRUCT_CTOR_6
#define OPENPBR_STRUCT_CTOR_6(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6)
#endif
#ifndef OPENPBR_STRUCT_CTOR_7
#define OPENPBR_STRUCT_CTOR_7(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7)
#endif
#ifndef OPENPBR_STRUCT_CTOR_8
#define OPENPBR_STRUCT_CTOR_8(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8)
#endif
#ifndef OPENPBR_STRUCT_CTOR_9
#define OPENPBR_STRUCT_CTOR_9(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9)
#endif
#ifndef OPENPBR_STRUCT_CTOR_10
#define OPENPBR_STRUCT_CTOR_10(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10)
#endif
#ifndef OPENPBR_STRUCT_CTOR_11
#define OPENPBR_STRUCT_CTOR_11(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11)
#endif
#ifndef OPENPBR_STRUCT_CTOR_12
#define OPENPBR_STRUCT_CTOR_12(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12)
#endif
#ifndef OPENPBR_STRUCT_CTOR_13
#define OPENPBR_STRUCT_CTOR_13(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13)
#endif
#ifndef OPENPBR_STRUCT_CTOR_14
#define OPENPBR_STRUCT_CTOR_14(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14)
#endif
#ifndef OPENPBR_STRUCT_CTOR_15
#define OPENPBR_STRUCT_CTOR_15(type, t1, n1, t2, n2, t3, n3, t4, n4, t5, n5, t6, n6, t7, n7, t8, n8, t9, n9, t10, n10, t11, n11, t12, n12, t13, n13, t14, n14, t15, n15)
#endif
#ifndef OPENPBR_UNROLL
#define OPENPBR_UNROLL
#endif
#ifndef OPENPBR_DONT_UNROLL
#define OPENPBR_DONT_UNROLL
#endif

#endif  // OPENPBR_INTEROP_H
