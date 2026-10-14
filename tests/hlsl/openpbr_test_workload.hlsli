// Shared workload for the OpenPBR HLSL compilation tests.
//
// Every test shader includes this after configuring its OpenPBR settings and
// including openpbr.h. It touches the whole public API - prepare / eval / sample
// / pdf - plus the prepared volume and emission outputs, so that DXC has to
// instantiate and codegen essentially all of the implementation. The result is
// folded into a single float4 that the caller writes out, which keeps DXC from
// dead-code-eliminating any of it.
//
// This is a compilation test, not a numerical one: the values are arbitrary but
// chosen to enable every lobe (metal, translucency, coat, fuzz, subsurface,
// thin film, dispersion) so no branch is trivially unreachable.

#ifndef OPENPBR_TEST_WORKLOAD_HLSLI
#define OPENPBR_TEST_WORKLOAD_HLSLI

OpenPBR_ResolvedInputs openpbr_test_make_inputs(const float seed)
{
    OpenPBR_ResolvedInputs inputs = openpbr_make_default_resolved_inputs();

    inputs.base_weight = 1.0f;
    inputs.base_color = vec3(0.72f, 0.24f, 0.11f);
    inputs.base_metalness = frac(seed * 0.37f);
    inputs.base_diffuse_roughness = 0.4f;

    inputs.specular_weight = 1.0f;
    inputs.specular_color = vec3(0.95f, 0.97f, 1.0f);
    inputs.specular_roughness = 0.05f + 0.9f * frac(seed * 0.11f);
    inputs.specular_roughness_anisotropy = 0.6f;
    inputs.specular_ior = 1.52f;

    // Translucency: drives the specular transmission lobe and the volume.
    inputs.transmission_weight = 0.5f;
    inputs.transmission_color = vec3(0.85f, 0.92f, 0.99f);
    inputs.transmission_depth = 0.3f;
    inputs.transmission_scatter = vec3(0.1f, 0.2f, 0.3f);
    inputs.transmission_scatter_anisotropy = 0.25f;
    // Non-zero dispersion exercises the dispersion code path.
    inputs.transmission_dispersion_scale = 0.5f;
    inputs.transmission_dispersion_abbe_number = 35.0f;

    // Subsurface.
    inputs.subsurface_weight = 0.35f;
    inputs.subsurface_color = vec3(0.9f, 0.55f, 0.45f);
    inputs.subsurface_radius = 0.6f;
    inputs.subsurface_radius_scale = vec3(1.0f, 0.5f, 0.25f);
    inputs.subsurface_scatter_anisotropy = 0.1f;

    // Coat and fuzz: the outer layers of the aggregate lobe.
    inputs.coat_weight = 0.75f;
    inputs.coat_color = vec3(0.98f, 0.99f, 1.0f);
    inputs.coat_roughness = 0.15f;
    inputs.coat_roughness_anisotropy = 0.3f;
    inputs.coat_ior = 1.6f;
    inputs.coat_darkening = 0.8f;

    inputs.fuzz_weight = 0.4f;
    inputs.fuzz_color = vec3(1.0f, 0.95f, 0.9f);
    inputs.fuzz_roughness = 0.5f;

    // Thin film.
    inputs.thin_film_weight = 0.6f;
    inputs.thin_film_thickness = 450.0f;
    inputs.thin_film_ior = 1.4f;

    inputs.emission_luminance = 2.0f;
    inputs.emission_color = vec3(0.4f, 0.6f, 1.0f);

    inputs.geometry_opacity = 1.0f;
    inputs.geometry_thin_walled = false;

    return inputs;
}

// Runs the full prepare / eval / pdf / sample sequence and accumulates every
// output into the returned value.
float4 openpbr_test_workload(const float seed)
{
    const OpenPBR_ResolvedInputs inputs = openpbr_test_make_inputs(seed);

    const vec3 view_direction = normalize(vec3(0.3f, 0.2f, 0.9f));
    const vec3 light_direction = normalize(vec3(-0.25f, 0.15f, 0.8f));
    const vec3 rgb_wavelengths_nm = vec3(612.0f, 549.0f, 465.0f);

    const OpenPBR_PreparedBsdf prepared =
        openpbr_prepare(inputs, vec3(1.0f), rgb_wavelengths_nm, 1.0f, view_direction);

    // eval + pdf
    const OpenPBR_DiffuseSpecular evaluated = openpbr_eval(prepared, light_direction);
    const float eval_pdf = openpbr_pdf(prepared, light_direction);

    // sample
    vec3 sampled_direction;
    OpenPBR_DiffuseSpecular sampled_weight;
    float sampled_pdf;
    OpenPBR_BsdfLobeType sampled_type;
    openpbr_sample(prepared, vec3(frac(seed * 0.13f), frac(seed * 0.29f), frac(seed * 0.71f)), sampled_direction,
                   sampled_weight, sampled_pdf, sampled_type);

    // Prepared-state outputs: emission and the homogeneous volume.
    const vec3 emission = prepared.emission;
    const vec3 volume_extinction = prepared.volume.extinction_coefficient;
    const vec3 volume_scattering_albedo = prepared.volume.albedo;

    vec3 rgb = evaluated.diffuse + evaluated.specular;
    rgb += sampled_weight.diffuse + sampled_weight.specular;
    rgb += sampled_direction;
    rgb += emission;
    rgb += volume_extinction + volume_scattering_albedo;

    const float alpha = eval_pdf + sampled_pdf + float(sampled_type) + float(prepared.volume.anisotropy);

    return float4(rgb, alpha);
}

#endif  // OPENPBR_TEST_WORKLOAD_HLSLI
