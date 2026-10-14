// Exercises the full OPENPBR_MAKE_STRUCT_N macro table.
//
// The OpenPBR structs only use arities 1, 2, 3, 4, 5 and 15, so the other
// macros would otherwise never be expanded by anything. This file builds a
// synthetic struct at every arity from 1 to 15 through the macros, so a typo
// in one of the unused rows of the table is a compile error here rather than a
// surprise the first time a new struct needs that arity.
//
// It also exercises OPENPBR_STATIC_ASSERT, which is a real compile-time check
// on this backend (DXC spells static_assert as _Static_assert).

#define OPENPBR_LANGUAGE_TARGET_HLSL 1
#include "openpbr.h"

RWStructuredBuffer<float4> Output : register(u0);

struct Arity1
{
    float f1;

    static Arity1 ctor(const float f1)
    {
        Arity1 result;
        result.f1 = f1;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity1) == 1 * 4, "unexpected Arity1 size");

struct Arity2
{
    float f1, f2;

    static Arity2 ctor(const float f1, const float f2)
    {
        Arity2 result;
        result.f1 = f1;
        result.f2 = f2;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity2) == 2 * 4, "unexpected Arity2 size");

struct Arity3
{
    float f1, f2, f3;

    static Arity3 ctor(const float f1, const float f2, const float f3)
    {
        Arity3 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity3) == 3 * 4, "unexpected Arity3 size");

struct Arity4
{
    float f1, f2, f3, f4;

    static Arity4 ctor(const float f1, const float f2, const float f3, const float f4)
    {
        Arity4 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity4) == 4 * 4, "unexpected Arity4 size");

struct Arity5
{
    float f1, f2, f3, f4, f5;

    static Arity5 ctor(const float f1, const float f2, const float f3, const float f4, const float f5)
    {
        Arity5 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity5) == 5 * 4, "unexpected Arity5 size");

struct Arity6
{
    float f1, f2, f3, f4, f5, f6;

    static Arity6 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6)
    {
        Arity6 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity6) == 6 * 4, "unexpected Arity6 size");

struct Arity7
{
    float f1, f2, f3, f4, f5, f6, f7;

    static Arity7 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7)
    {
        Arity7 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity7) == 7 * 4, "unexpected Arity7 size");

struct Arity8
{
    float f1, f2, f3, f4, f5, f6, f7, f8;

    static Arity8 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8)
    {
        Arity8 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity8) == 8 * 4, "unexpected Arity8 size");

struct Arity9
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9;

    static Arity9 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9)
    {
        Arity9 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity9) == 9 * 4, "unexpected Arity9 size");

struct Arity10
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10;

    static Arity10 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9, const float f10)
    {
        Arity10 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        result.f10 = f10;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity10) == 10 * 4, "unexpected Arity10 size");

struct Arity11
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11;

    static Arity11 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9, const float f10, const float f11)
    {
        Arity11 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        result.f10 = f10;
        result.f11 = f11;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity11) == 11 * 4, "unexpected Arity11 size");

struct Arity12
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12;

    static Arity12 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9, const float f10, const float f11, const float f12)
    {
        Arity12 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        result.f10 = f10;
        result.f11 = f11;
        result.f12 = f12;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity12) == 12 * 4, "unexpected Arity12 size");

struct Arity13
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13;

    static Arity13 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9, const float f10, const float f11, const float f12, const float f13)
    {
        Arity13 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        result.f10 = f10;
        result.f11 = f11;
        result.f12 = f12;
        result.f13 = f13;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity13) == 13 * 4, "unexpected Arity13 size");

struct Arity14
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14;

    static Arity14 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9, const float f10, const float f11, const float f12, const float f13, const float f14)
    {
        Arity14 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        result.f10 = f10;
        result.f11 = f11;
        result.f12 = f12;
        result.f13 = f13;
        result.f14 = f14;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity14) == 14 * 4, "unexpected Arity14 size");

struct Arity15
{
    float f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15;

    static Arity15 ctor(const float f1, const float f2, const float f3, const float f4, const float f5, const float f6, const float f7, const float f8, const float f9, const float f10, const float f11, const float f12, const float f13, const float f14, const float f15)
    {
        Arity15 result;
        result.f1 = f1;
        result.f2 = f2;
        result.f3 = f3;
        result.f4 = f4;
        result.f5 = f5;
        result.f6 = f6;
        result.f7 = f7;
        result.f8 = f8;
        result.f9 = f9;
        result.f10 = f10;
        result.f11 = f11;
        result.f12 = f12;
        result.f13 = f13;
        result.f14 = f14;
        result.f15 = f15;
        return result;
    }
};
OPENPBR_STATIC_ASSERT(sizeof(Arity15) == 15 * 4, "unexpected Arity15 size");

[numthreads(64, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
    const float s = float(thread_id.x);
    float total = 0.0f;

    total += OPENPBR_MAKE_STRUCT_1(Arity1, s + 1.0f).f1;
    total += OPENPBR_MAKE_STRUCT_2(Arity2, s + 1.0f, s + 2.0f).f2;
    total += OPENPBR_MAKE_STRUCT_3(Arity3, s + 1.0f, s + 2.0f, s + 3.0f).f3;
    total += OPENPBR_MAKE_STRUCT_4(Arity4, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f).f4;
    total += OPENPBR_MAKE_STRUCT_5(Arity5, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f).f5;
    total += OPENPBR_MAKE_STRUCT_6(Arity6, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f).f6;
    total += OPENPBR_MAKE_STRUCT_7(Arity7, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f).f7;
    total += OPENPBR_MAKE_STRUCT_8(Arity8, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f).f8;
    total += OPENPBR_MAKE_STRUCT_9(Arity9, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f).f9;
    total += OPENPBR_MAKE_STRUCT_10(Arity10, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f, s + 10.0f).f10;
    total += OPENPBR_MAKE_STRUCT_11(Arity11, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f, s + 10.0f, s + 11.0f).f11;
    total += OPENPBR_MAKE_STRUCT_12(Arity12, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f, s + 10.0f, s + 11.0f, s + 12.0f).f12;
    total += OPENPBR_MAKE_STRUCT_13(Arity13, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f, s + 10.0f, s + 11.0f, s + 12.0f, s + 13.0f).f13;
    total += OPENPBR_MAKE_STRUCT_14(Arity14, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f, s + 10.0f, s + 11.0f, s + 12.0f, s + 13.0f, s + 14.0f).f14;
    total += OPENPBR_MAKE_STRUCT_15(Arity15, s + 1.0f, s + 2.0f, s + 3.0f, s + 4.0f, s + 5.0f, s + 6.0f, s + 7.0f, s + 8.0f, s + 9.0f, s + 10.0f, s + 11.0f, s + 12.0f, s + 13.0f, s + 14.0f, s + 15.0f).f15;

    Output[thread_id.x] = float4(total, total, total, total);
}
