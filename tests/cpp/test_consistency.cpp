// Consistency test for the C++ backend: the API entry points that are defined to
// agree with each other do, over randomized materials and directions.
//
//   - openpbr_eval_and_pdf equals openpbr_eval and openpbr_pdf, bit for bit;
//   - openpbr_sample_direction picks the same direction, lobe type and validity as
//     openpbr_sample (and the same weight and pdf for delta samples);
//   - with OPENPBR_SAMPLE_REEVALUATE = 1, a non-delta sample's pdf is openpbr_pdf in
//     its direction and its weight is openpbr_eval / openpbr_pdf there.
//
// Build and run it once per configuration:
//   g++ -std=c++17 -O2 -I. -I/path/to/glm tests/cpp/test_consistency.cpp -o test && ./test
//   g++ -std=c++17 -O2 -I. -I/path/to/glm -DOPENPBR_SAMPLE_REEVALUATE=1 tests/cpp/test_consistency.cpp -o test && ./test
//   g++ -std=c++17 -O2 -I. -I/path/to/glm -DOPENPBR_SKIP_INACTIVE_LOBES=1 tests/cpp/test_consistency.cpp -o test && ./test
// Exit code 0 means every check passed.

#include <glm/glm.hpp>

#include "openpbr.h"

#include <cmath>
#include <cstdio>
#include <random>

static std::mt19937 g_rng{20261106u};

static float uniform(float lo = 0.0f, float hi = 1.0f)
{
    return std::uniform_real_distribution<float>(lo, hi)(g_rng);
}

// A weight that is often exactly 0 or 1, the values the code special-cases.
static float weight()
{
    const float r = uniform();
    return r < 0.3f ? 0.0f : (r < 0.45f ? 1.0f : uniform());
}

static vec3 color()
{
    return vec3(uniform(), uniform(), uniform());
}

static vec3 direction()
{
    for (;;)
    {
        const vec3 d(uniform(-1, 1), uniform(-1, 1), uniform(-1, 1));
        const float l = glm::length(d);
        if (l > 0.05f && l <= 1.0f)
            return d / l;
    }
}

static bool same(float a, float b)
{
    return a == b || (std::isnan(a) && std::isnan(b));
}

static bool same(const vec3 a, const vec3 b)
{
    return same(a.x, b.x) && same(a.y, b.y) && same(a.z, b.z);
}

static bool same(const OpenPBR_DiffuseSpecular& a, const OpenPBR_DiffuseSpecular& b)
{
    return same(a.diffuse, b.diffuse) && same(a.specular, b.specular);
}

static bool close(float a, float b)
{
    return std::abs(a - b) <= 1e-5f * std::max(std::abs(a), std::abs(b)) || same(a, b);
}

static OpenPBR_ResolvedInputs random_inputs()
{
    OpenPBR_ResolvedInputs in = openpbr_make_default_resolved_inputs();
    in.base_weight = weight();
    in.base_color = color();
    in.base_diffuse_roughness = uniform();
    in.base_metalness = weight();
    in.subsurface_weight = weight();
    in.subsurface_color = color();
    in.specular_weight = weight();
    in.specular_color = color();
    in.specular_roughness = uniform() < 0.15f ? 0.0f : uniform();
    in.specular_roughness_anisotropy = uniform() < 0.5f ? 0.0f : uniform();
    in.specular_ior = uniform(1.0f, 2.5f);
    in.coat_weight = weight();
    in.coat_roughness = uniform() < 0.15f ? 0.0f : uniform();
    in.coat_ior = uniform(1.0f, 2.2f);
    in.fuzz_weight = weight();
    in.fuzz_roughness = uniform();
    in.transmission_weight = weight();
    in.transmission_dispersion_scale = uniform() < 0.5f ? 0.0f : uniform();
    in.thin_film_weight = weight();
    in.thin_film_thickness = uniform(0.0f, 2.0f);
    in.geometry_thin_walled = uniform() < 0.25f;
    return in;
}

int main()
{
    int failures = 0;
    int checked = 0;
    for (int i = 0; i < 20000; ++i)
    {
        const OpenPBR_ResolvedInputs in = random_inputs();
        vec3 view = direction();
        if (view.z < 0.0f && uniform() < 0.8f)
            view.z = -view.z;  // mostly outside, sometimes inside
        const vec3 throughput = uniform() < 0.5f ? vec3(1.0f) : vec3(uniform(0.05f, 1), uniform(0.05f, 1), uniform(0.05f, 1));
        const vec3 wavelengths = uniform() < 0.5f ? OpenPBR_BaseRgbWavelengths_nm : vec3(uniform(380, 780), uniform(380, 780), uniform(380, 780));
        const OpenPBR_PreparedBsdf prepared = openpbr_prepare(in, throughput, wavelengths, uniform() < 0.7f ? 1.0f : uniform(1.0f, 1.6f), view);

        // eval_and_pdf == eval + pdf
        const vec3 light = direction();
        OpenPBR_DiffuseSpecular value;
        float pdf;
        openpbr_eval_and_pdf(prepared, light, value, pdf);
        if (!same(value, openpbr_eval(prepared, light)) || !same(pdf, openpbr_pdf(prepared, light)))
        {
            if (++failures <= 10)
                std::printf("case %d: openpbr_eval_and_pdf differs from openpbr_eval / openpbr_pdf\n", i);
        }

        // sample_direction == sample, in direction, type and validity
        const vec3 rand(uniform(0, 0.9999f), uniform(0, 0.9999f), uniform(0, 0.9999f));
        vec3 sampled_direction, only_direction;
        OpenPBR_DiffuseSpecular sampled_weight, delta_weight;
        float sampled_pdf, delta_pdf;
        OpenPBR_BsdfLobeType sampled_type, only_type;
        openpbr_sample(prepared, rand, sampled_direction, sampled_weight, sampled_pdf, sampled_type);
        openpbr_sample_direction(prepared, rand, only_direction, delta_weight, delta_pdf, only_type);
        const bool delta = bool(only_type & OpenPBR_BsdfLobeTypeSpecular);
        if (sampled_pdf > 0.0f)
        {
            if (!same(sampled_direction, only_direction) || sampled_type != only_type)
            {
                if (++failures <= 10)
                    std::printf("case %d: openpbr_sample_direction picked another direction or lobe than openpbr_sample\n", i);
            }
            else if (delta && (!same(sampled_weight, delta_weight) || !same(sampled_pdf, delta_pdf)))
            {
                if (++failures <= 10)
                    std::printf("case %d: openpbr_sample_direction's delta weight or pdf differs from openpbr_sample's\n", i);
            }
        }

        // With OPENPBR_SAMPLE_REEVALUATE, a non-delta sample is the evaluation of its direction.
        if (OPENPBR_SAMPLE_REEVALUATE_ENABLED && sampled_pdf > 0.0f && !bool(sampled_type & OpenPBR_BsdfLobeTypeSpecular))
        {
            const OpenPBR_DiffuseSpecular e = openpbr_eval(prepared, sampled_direction);
            const float p = openpbr_pdf(prepared, sampled_direction);
            if (!same(sampled_pdf, p) || !close(sampled_weight.specular.x, e.specular.x / p) || !close(sampled_weight.diffuse.x, e.diffuse.x / p))
            {
                if (++failures <= 10)
                    std::printf("case %d: a reevaluated sample is not openpbr_eval / openpbr_pdf in its direction\n", i);
            }
        }
        ++checked;
    }
    std::printf("%d cases, %d failures (OPENPBR_SAMPLE_REEVALUATE=%d, OPENPBR_SKIP_INACTIVE_LOBES=%d)\n", checked, failures,
                int(OPENPBR_SAMPLE_REEVALUATE), int(OPENPBR_SKIP_INACTIVE_LOBES));
    return failures == 0 ? 0 : 1;
}
