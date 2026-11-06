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

#ifndef OPENPBR_COMPREHENSIVE_MICROFACET_LOBE_H
#define OPENPBR_COMPREHENSIVE_MICROFACET_LOBE_H

#include "openpbr_reflection_transmission_coefficient.h"

#include "../openpbr_constants.h"
#include "../openpbr_diffuse_specular.h"
#include "openpbr_dispersion_utils.h"
#include "openpbr_lobe_utils.h"
#include "openpbr_vndf_microfacet_distribution.h"

///////////////////////////////////////////////////////
// ComprehensiveMicrofacetReflectionTransmissionLobe //
///////////////////////////////////////////////////////

struct OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe
{
    vec3 normal_ff;
    OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution microfacet_distr;
    OpenPBR_ComprehensiveReflectionTransmissionCoefficient refl_trans_coeff;
    vec3 eta_t_over_eta_i;
    vec3 path_throughput;

    OPENPBR_STRUCT_CTOR_5(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe,
        vec3, normal_ff,
        OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution, microfacet_distr,
        OpenPBR_ComprehensiveReflectionTransmissionCoefficient, refl_trans_coeff,
        vec3, eta_t_over_eta_i,
        vec3, path_throughput)
};

vec3 openpbr_channel_probabilities(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe)
{
    return lobe.path_throughput / openpbr_sum(lobe.path_throughput);
}

bool openpbr_validate_half_vector_for_transmission(
    OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
    const vec3 half_vector)
{
    // The computed half vector should point into the less dense medium.
    // If the face-forwarded normal points into the less dense medium, half_vector is valid if it faces forward into the less dense medium.
    // If the face-forwarded normal points into the denser medium, half_vector is valid if it faces backward into the less dense medium.
    return (lobe.eta_t_over_eta_i.r >= 1.0f && dot(lobe.normal_ff, half_vector) > 0.0f) ||
           (lobe.eta_t_over_eta_i.r < 1.0f && dot(lobe.normal_ff, half_vector) < 0.0f);
}

// Whether this lobe's refraction actually differs per channel. With dispersion
// compiled in (EnableDispersion) but a material without it, the three channels'
// relative IORs are equal, their refraction half vectors identical, and a
// per-channel loop would compute the same coefficients - the lobe's most
// expensive part - three times; such a loop runs once and broadcasts instead.
bool openpbr_is_dispersive(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe)
{
    return lobe.eta_t_over_eta_i.g != lobe.eta_t_over_eta_i.r || lobe.eta_t_over_eta_i.b != lobe.eta_t_over_eta_i.r;
}

void openpbr_initialize_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_INOUT(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                             const vec3 normal_ff,
                             const OpenPBR_AnisotropicGGXSmithVNDFMicrofacetDistribution microfacet_distr,
                             const OpenPBR_ComprehensiveReflectionTransmissionCoefficient refl_trans_coeff,
                             const vec3 eta_t_over_eta_i,
                             const vec3 path_throughput)
{
    OPENPBR_ASSERT(openpbr_is_normalized(normal_ff), "The input normal is assumed to be normalized");

    lobe = OPENPBR_MAKE_STRUCT_5(
        OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe, normal_ff, microfacet_distr, refl_trans_coeff, eta_t_over_eta_i, path_throughput);
}

OpenPBR_BsdfLobeType openpbr_get_lobe_type(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe)
                                               lobe)
{
    return OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency)
               ? OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection | OpenPBR_BsdfLobeTypeTransmission
               : OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;
}

OpenPBR_DiffuseSpecular
openpbr_calculate_lobe_value(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                             const vec3 view_direction,
                             const vec3 light_direction)
{
    const float idotn = dot(lobe.normal_ff, view_direction);
    const float odotn = dot(lobe.normal_ff, light_direction);

    if (idotn * odotn > 0.0f)  // reflection
    {
        const vec3 half_vector = openpbr_fast_normalize(view_direction + light_direction);

        const float idoth = dot(view_direction, half_vector);

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        const float other_terms = 1.0f / (4.0f * idotn);
        const vec3 F = openpbr_reflection_coefficient(lobe.refl_trans_coeff, abs(idoth));

        const vec3 D_other_terms_F = D * other_terms * F;

        const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);

        return openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);
    }
    else if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && idotn * odotn < 0.0f)  // transmission
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion))
        {
            // Rolled, and nothing indexed by the counter but through openpbr_component:
            // the body holds the whole transmission coefficient (thin film included).
            // Once, for all channels, when they refract alike (openpbr_is_dispersive).
            vec3 D_other_terms_F = vec3(0.0f);  // 0 for a channel with invalid transmission
            const bool dispersive = openpbr_is_dispersive(lobe);
            OPENPBR_DONT_UNROLL
            for (int channel = 0; channel < (dispersive ? OpenPBR_NumRgbChannels : 1); ++channel)
            {
                const float eta = openpbr_component(lobe.eta_t_over_eta_i, channel);
                const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * eta);
                if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                    continue;  // invalid transmission

                const float idoth = dot(view_direction, half_vector);
                const float odoth = dot(light_direction, half_vector);
                if (idoth * odoth >= 0.0f)
                    continue;  // invalid transmission

                const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
                const float other_terms =
                    openpbr_square(eta) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + eta * odoth) * idotn);
                const vec3 F = openpbr_transmission_coefficient(lobe.refl_trans_coeff, abs(idoth));

                // Per component the same product either way, so the broadcast is exact.
                D_other_terms_F = dispersive ? openpbr_with_component(D_other_terms_F, channel, D * other_terms * openpbr_component(F, channel))
                                             : D * other_terms * F;
            }

            const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);

            return openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);
        }
        else
        {
            const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * lobe.eta_t_over_eta_i.r);
            if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                return openpbr_make_zero_diffuse_specular();  // invalid transmission

            const float idoth = dot(view_direction, half_vector);
            const float odoth = dot(light_direction, half_vector);
            if (idoth * odoth >= 0.0f)
                return openpbr_make_zero_diffuse_specular();  // invalid transmission

            const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
            const float other_terms =
                openpbr_square(lobe.eta_t_over_eta_i.r) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + lobe.eta_t_over_eta_i.r * odoth) * idotn);
            const vec3 F = openpbr_transmission_coefficient(lobe.refl_trans_coeff, abs(idoth));

            const vec3 D_other_terms_F = D * other_terms * F;

            const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);

            return openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);
        }
    }
    else  // neither reflection nor transmission
        return openpbr_make_zero_diffuse_specular();

    // If cosine term needs to be excluded, code like this could be used:
    //     if (!IncludeCosineTerm)
    //         result /= odotn;
}

float openpbr_calculate_lobe_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                                 const vec3 view_direction,
                                 const vec3 light_direction)
{
    const float idotn = dot(lobe.normal_ff, view_direction);
    const float odotn = dot(lobe.normal_ff, light_direction);

    if (idotn * odotn > 0.0f)  // reflection
    {
        const vec3 half_vector = openpbr_fast_normalize(view_direction + light_direction);

        const float idoth = dot(view_direction, half_vector);
        const float odoth = dot(light_direction, half_vector);

        // Return invalid PDF if half vector rounding error would result in negative PDF
        if (idoth * odoth < 0.0f)
            return 0.0f;

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        const float other_terms = idoth / (4.0f * odoth * idotn);
        const float F_prob = openpbr_reflection_probability(lobe.refl_trans_coeff, lobe.path_throughput, abs(idoth));

        const float F_prob_D_other_terms = F_prob * D * other_terms;

        const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

        return G_masking * F_prob_D_other_terms;
    }
    else if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && idotn * odotn < 0.0f)  // transmission
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion))
        {
            // As in openpbr_calculate_lobe_value: rolled, components through openpbr_component,
            // and once for all channels when they refract alike.
            vec3 F_prob_D_other_terms = vec3(0.0f);  // 0 for a channel with invalid transmission
            const bool dispersive = openpbr_is_dispersive(lobe);
            OPENPBR_DONT_UNROLL
            for (int channel = 0; channel < (dispersive ? OpenPBR_NumRgbChannels : 1); ++channel)
            {
                const float eta = openpbr_component(lobe.eta_t_over_eta_i, channel);
                const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * eta);
                if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                    continue;  // invalid transmission

                const float idoth = dot(view_direction, half_vector);
                const float odoth = dot(light_direction, half_vector);
                if (idoth * odoth >= 0.0f)
                    continue;  // invalid transmission

                const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
                const float other_terms =
                    openpbr_square(eta) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + eta * odoth) * idotn);
                const float F_prob = openpbr_transmission_probability(lobe.refl_trans_coeff, lobe.path_throughput, abs(idoth));

                F_prob_D_other_terms = dispersive ? openpbr_with_component(F_prob_D_other_terms, channel, F_prob * D * other_terms)
                                                  : vec3(F_prob * D * other_terms);
            }

            const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

            return G_masking * dot(F_prob_D_other_terms, openpbr_channel_probabilities(lobe));
        }
        else
        {
            const vec3 half_vector = -openpbr_fast_normalize(view_direction + light_direction * lobe.eta_t_over_eta_i.r);
            if (!openpbr_validate_half_vector_for_transmission(lobe, half_vector))
                return 0.0f;  // invalid transmission

            const float idoth = dot(view_direction, half_vector);
            const float odoth = dot(light_direction, half_vector);
            if (idoth * odoth >= 0.0f)
                return 0.0f;  // invalid transmission

            const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
            const float other_terms =
                openpbr_square(lobe.eta_t_over_eta_i.r) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + lobe.eta_t_over_eta_i.r * odoth) * idotn);
            const float F_prob = openpbr_transmission_probability(lobe.refl_trans_coeff, lobe.path_throughput, abs(idoth));

            const float F_prob_D_other_terms = F_prob * D * other_terms;

            const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);

            return G_masking * F_prob_D_other_terms;
        }
    }
    else  // neither reflection nor transmission
        return 0.0f;
}

// Value and pdf toward one light direction in one pass - see
// openpbr_calculate_lobe_value_and_pdf in openpbr_aggregate_lobe.h. Results are
// those of openpbr_calculate_lobe_value and _pdf.
//
// The two share the half vector(s), D, and above all the Fresnel coefficients,
// thin film included: in reflection the value's reflection coefficient and the
// pdf's reflection probability come from one openpbr_all_coefficients
// evaluation (openpbr_reflection_coefficient is exactly its reflection half);
// in transmission the value's coefficient and the pdf's probability come from
// one openpbr_all_coefficients_with_value_transmission evaluation.
//
// Reflection and both kinds of transmission also share one loop: an
// evaluation point per iteration - the one reflection half vector, or one
// refraction half vector per channel with dispersion (else one) - so the
// coefficients, the most expensive part of the lobe, exist once in the code
// rather than once per case.
void openpbr_calculate_lobe_value_and_pdf(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                                          const vec3 view_direction,
                                          const vec3 light_direction,
                                          OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) value,
                                          OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf)
{
#ifdef ECLAIR_RASTERIZER
    // openpbr_reflection_coefficient desaturates the thin film's reflectance in this
    // build and openpbr_all_coefficients does not, so the shared evaluation below
    // would not reproduce the value: evaluate the two separately.
    value = openpbr_calculate_lobe_value(lobe, view_direction, light_direction);
    pdf = openpbr_calculate_lobe_pdf(lobe, view_direction, light_direction);
    return;
#endif
    const float idotn = dot(lobe.normal_ff, view_direction);
    const float odotn = dot(lobe.normal_ff, light_direction);

    const bool reflection = idotn * odotn > 0.0f;
    const bool transmission = OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) && idotn * odotn < 0.0f;
    if (!reflection && !transmission)  // neither reflection nor transmission
    {
        value = openpbr_make_zero_diffuse_specular();
        pdf = 0.0f;
        return;
    }
    const bool per_channel = transmission && OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion);
    // Per channel, but all channels refracting alike: one iteration, broadcast.
    const bool dispersive = per_channel && openpbr_is_dispersive(lobe);

    // Reflection and transmission without dispersion fill the scalar/vec3 forms
    // of openpbr_calculate_lobe_value/_pdf directly; per-channel transmission
    // builds these up a channel at a time (0 for a channel with invalid
    // transmission).
    vec3 D_other_terms_F = vec3(0.0f);
    vec3 F_prob_D_other_terms_per_channel = vec3(0.0f);
    float F_prob_D_other_terms = 0.0f;
    bool valid = false;           // reflection / single transmission: the evaluation point was usable
    bool reflection_pdf_valid = true;  // reflection only: the pdf's half-vector rounding guard

    OPENPBR_DONT_UNROLL
    for (int channel = 0; channel < (dispersive ? OpenPBR_NumRgbChannels : 1); ++channel)
    {
        const float eta = per_channel ? openpbr_component(lobe.eta_t_over_eta_i, channel) : lobe.eta_t_over_eta_i.r;
        const vec3 half_vector = reflection ? openpbr_fast_normalize(view_direction + light_direction)
                                            : -openpbr_fast_normalize(view_direction + light_direction * eta);
        if (transmission && !openpbr_validate_half_vector_for_transmission(lobe, half_vector))
            continue;  // invalid transmission

        const float idoth = dot(view_direction, half_vector);
        const float odoth = dot(light_direction, half_vector);
        if (transmission && idoth * odoth >= 0.0f)
            continue;  // invalid transmission

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        vec3 transmission_F;
        const OpenPBR_AllCoefficients coefficients =
            openpbr_all_coefficients_with_value_transmission(lobe.refl_trans_coeff, abs(idoth), transmission_F);

        if (reflection)
        {
            // As openpbr_calculate_lobe_value.
            const float other_terms = 1.0f / (4.0f * idotn);
            D_other_terms_F = D * other_terms * coefficients.reflection_coefficient;
            // As openpbr_calculate_lobe_pdf, including its rounding guard.
            reflection_pdf_valid = !(idoth * odoth < 0.0f);
            if (reflection_pdf_valid)
            {
                const float pdf_other_terms = idoth / (4.0f * odoth * idotn);
                const float F_prob = openpbr_reflection_probability_from_coefficients(coefficients, lobe.path_throughput);
                F_prob_D_other_terms = F_prob * D * pdf_other_terms;
            }
        }
        else
        {
            const float other_terms = openpbr_square(eta) * abs(idoth) * abs(odoth) / (openpbr_square(idoth + eta * odoth) * idotn);
            const float F_prob = openpbr_transmission_probability_from_coefficients(coefficients, lobe.path_throughput);
            if (dispersive)
            {
                D_other_terms_F = openpbr_with_component(D_other_terms_F, channel, D * other_terms * openpbr_component(transmission_F, channel));
                F_prob_D_other_terms_per_channel = openpbr_with_component(F_prob_D_other_terms_per_channel, channel, F_prob * D * other_terms);
            }
            else if (per_channel)  // every channel the same: exactly what the per-channel products would be
            {
                D_other_terms_F = D * other_terms * transmission_F;
                F_prob_D_other_terms_per_channel = vec3(F_prob * D * other_terms);
            }
            else
            {
                D_other_terms_F = D * other_terms * transmission_F;
                F_prob_D_other_terms = F_prob * D * other_terms;
            }
        }
        valid = true;
    }

    if (!per_channel && !valid)  // invalid transmission
    {
        value = openpbr_make_zero_diffuse_specular();
        pdf = 0.0f;
        return;
    }

    const float G = openpbr_eval_smith_g2(lobe.microfacet_distr, view_direction, light_direction, idotn, odotn);
    value = openpbr_make_diffuse_specular_from_specular(G * D_other_terms_F);

    if (reflection && !reflection_pdf_valid)
    {
        pdf = 0.0f;
        return;
    }
    const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);
    pdf = per_channel ? G_masking * dot(F_prob_D_other_terms_per_channel, openpbr_channel_probabilities(lobe))
                      : G_masking * F_prob_D_other_terms;
}

bool openpbr_sample_lobe(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe) lobe,
                         const vec3 rand,
                         const vec3 view_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(vec3) light_direction,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_DiffuseSpecular) weight,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(float) pdf,
                         OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_OUT(OpenPBR_BsdfLobeType) sampled_type)
{
    const float idotn = dot(view_direction, lobe.normal_ff);

    // Choose microfacet normal_ff (in the caller's coordinate space).
    const vec3 half_vector = openpbr_sample_ggx_smith_vndf(lobe.microfacet_distr, view_direction, lobe.normal_ff, OPENPBR_SWIZZLE(rand, xy));

    const float idoth = dot(view_direction, half_vector);
    // Sampled microfacet normal is expected to be in incident hemisphere,
    // but occasionally is not due to rounding error.
    if (idoth < 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    // Select reflection or transmission.
    OpenPBR_AllCoefficientsAndProbabilities coeffs_and_probs =
        openpbr_all_coefficients_and_probabilities(lobe.refl_trans_coeff, lobe.path_throughput, idoth);
    const vec3 F_refl = coeffs_and_probs.reflection_coefficient;
    const vec3 F_trans = coeffs_and_probs.transmission_coefficient;
    const float P_refl = coeffs_and_probs.reflection_probability;
    const float P_trans = coeffs_and_probs.transmission_probability;

    if (P_refl + P_trans == 0.0f)
    {
        openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
        return false;
    }

    if (!OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableTranslucency) || rand.z < P_refl)  // reflection
    {
        // The light direction is normalized by construction, but can be significantly off due to rounding error.
        light_direction = openpbr_fast_normalize(-view_direction + half_vector * 2.0f * idoth);

        // Check if reflection is on correct side of surface.
        const float odotn = dot(lobe.normal_ff, light_direction);
        if (idotn * odotn <= 0.0f)
        {
            openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
            return false;
        }

        const float odoth = dot(light_direction, half_vector);

        // If the input normal and view direction are normalized, this assertion should never fail.
        OPENPBR_ASSERT(odoth >= 0.0f, "odoth is expected to be non-negative");

        const float G_shadowing = openpbr_eval_smith_g1(lobe.microfacet_distr, light_direction, odotn);
        weight = openpbr_make_diffuse_specular_from_specular((F_refl * (1.0f / P_refl)) * G_shadowing);

        const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
        if (D <= 0.0f)
        {
            openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
            return false;
        }

        const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);
        pdf = P_refl * D * G_masking * idoth / (4.0f * odoth * idotn);

        // It's very important to return the correct type to ensure correct treatment by outside systems.
        sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeReflection;

        return true;
    }
    else  // transmission
    {
        if (OPENPBR_GET_SPECIALIZATION_CONSTANT(EnableDispersion))
        {
            const float remapped_rand_z = (rand.z - P_refl) / P_trans;
            int selected_color_channel;
            const vec3 color_channel_probabilities = openpbr_channel_probabilities(lobe);
            if (remapped_rand_z < color_channel_probabilities[0])
                selected_color_channel = 0;
            else if (remapped_rand_z < color_channel_probabilities[0] + color_channel_probabilities[1])
                selected_color_channel = 1;
            else
                // TODO: Address numerical vulnerabilities.
                selected_color_channel = 2;

            if (!openpbr_refract(view_direction, half_vector, idoth, openpbr_component(lobe.eta_t_over_eta_i, selected_color_channel), light_direction))
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            // Check if transmission is on correct side of surface.
            const float odotn = dot(light_direction, lobe.normal_ff);
            if (idotn * odotn >= 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            // TODO: Only perform the necessary calculations.
            if (OPENPBR_SAMPLE_REEVALUATE_ENABLED)
            {
                // The caller evaluates the direction; a positive placeholder pdf marks
                // the sample valid (the evaluation rejects it if its pdf is zero).
                pdf = 1.0f;
                weight = openpbr_make_zero_diffuse_specular();
            }
            else
            {
                OpenPBR_DiffuseSpecular bsdf_cos;
                openpbr_calculate_lobe_value_and_pdf(lobe, view_direction, light_direction, bsdf_cos, pdf);
                if (pdf == 0.0f)
                {
                    openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                    return false;
                }
                weight = openpbr_scale_diffuse_specular(bsdf_cos, 1.0f / pdf);
            }

            // It's very important to return the correct type to ensure correct treatment by outside systems.
            sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeTransmission;

            return true;
        }
        else
        {
            if (!openpbr_refract(view_direction, half_vector, idoth, lobe.eta_t_over_eta_i.r, light_direction))
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            // Check if transmission is on correct side of surface.
            const float odotn = dot(light_direction, lobe.normal_ff);
            if (idotn * odotn >= 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            const float odoth = dot(light_direction, half_vector);

            const float G_shadowing = openpbr_eval_smith_g1(lobe.microfacet_distr, light_direction, odotn);
            weight = openpbr_make_diffuse_specular_from_specular((F_trans * (1.0f / P_trans)) * G_shadowing);

            const float D = openpbr_eval_ggx(lobe.microfacet_distr, half_vector, lobe.normal_ff);
            if (D <= 0.0f)
            {
                openpbr_clear_lobe_sampling_output(light_direction, weight, pdf, sampled_type);
                return false;
            }

            const float G_masking = openpbr_eval_smith_g1(lobe.microfacet_distr, view_direction, idotn);
            pdf = P_trans * D * G_masking * openpbr_square(lobe.eta_t_over_eta_i.r) * abs(idoth) * abs(odoth) /
                  (openpbr_square(idoth + lobe.eta_t_over_eta_i.r * odoth) * idotn);

            // It's very important to return the correct type to ensure correct treatment by outside systems.
            sampled_type = OpenPBR_BsdfLobeTypeGlossy | OpenPBR_BsdfLobeTypeTransmission;

            return true;
        }
    }
}

float openpbr_estimate_lobe_contribution(OPENPBR_ADDRESS_SPACE_THREAD OPENPBR_CONST_REF(OpenPBR_ComprehensiveMicrofacetReflectionTransmissionLobe)
                                             lobe,
                                         const vec3 view_direction,
                                         const vec3 path_throughput)
{
    const float idotn = dot(view_direction, lobe.normal_ff);
    // Here we assume that the microfacet distribution is a GGX one and calculate the estimated weight accordingly.
    return openpbr_estimate_weight_when_applied_to_ggx_microfacet_distribution(
        lobe.refl_trans_coeff, path_throughput, idotn, openpbr_get_isotropic_alpha(lobe.microfacet_distr));
}

#endif  // !OPENPBR_COMPREHENSIVE_MICROFACET_LOBE_H
