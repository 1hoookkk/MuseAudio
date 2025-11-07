#pragma once
#include "BiquadCascade.h"
#include <cmath>

namespace st
{
    // Transform biquad state to maintain output continuity when coefficients change
    // Solves for new state that produces the same output with new coefficients
    inline void retargetBiquad(
        const BiquadSection& oldCoeffs,
        const BiquadSection& newCoeffs,
        BiquadSection& state) // state.z1, state.z2 are the state variables
    {
        // Old coefficients: a1_old, a2_old, b0_old, b1_old, b2_old
        // New coefficients: a1_new, a2_new, b0_new, b1_new, b2_new
        // State variables: z1, z2 (stored in state.z1, state.z2)

        // Current output using old coefficients:
        // y[n] = b0_old*x[n] + b1_old*x[n-1] + b2_old*x[n-2] - a1_old*y[n-1] - a2_old*y[n-2]
        // But we only have state variables, not input history

        // State space representation:
        // z1[n] = b1_old*x[n-1] - a1_old*y[n-1] + z2[n-1]
        // z2[n] = b2_old*x[n-2] - a2_old*y[n-2]

        // We want new state z1_new, z2_new such that the output remains continuous
        // This is a 2x2 linear system we need to solve

        const float a1_old = oldCoeffs.a1;
        const float a2_old = oldCoeffs.a2;
        const float a1_new = newCoeffs.a1;
        const float a2_new = newCoeffs.a2;

        // Build the 2x2 matrix for the state transformation
        // [z1_new] = M * [z1_old]
        // [z2_new]       [z2_old]

        // Derivation from state-space equations:
        // We want: y[n]_new = y[n]_old
        // This leads to a linear system that maps old state to new state

        // Simplified solution using direct form II transposed structure
        const float det = 1.0f + a1_new * a1_old + a2_new * a2_old;

        if (std::abs(det) > 1e-10f) // Ensure matrix is invertible
        {
            // Transform state to maintain output continuity
            const float z1_old = state.z1;
            const float z2_old = state.z2;

            // Solve the 2x2 system
            state.z1 = (z1_old - a1_new * z2_old) / det;
            state.z2 = (z2_old - a2_new * z1_old) / det;
        }
        // else: keep current state (potential for small click)
    }

    // Apply state transformation to an entire cascade
    inline void retargetCascade(
        const BiquadCascade6& oldCascade,
        const BiquadCascade6& newCascade,
        BiquadCascade6& stateCascade)
    {
        for (int i = 0; i < 6; ++i)
        {
            retargetBiquad(oldCascade.s[i], newCascade.s[i], stateCascade.s[i]);
        }
    }

    // Utility: check if coefficient change is significant enough to warrant state transform
    inline bool needsStateTransform(const BiquadSection& a, const BiquadSection& b, float threshold = 1e-6f)
    {
        return (std::abs(a.a1 - b.a1) > threshold) ||
               (std::abs(a.a2 - b.a2) > threshold) ||
               (std::abs(a.b0 - b.b0) > threshold) ||
               (std::abs(a.b1 - b.b1) > threshold) ||
               (std::abs(a.b2 - b.b2) > threshold);
    }

    // Utility: check if cascade needs state transformation
    inline bool needsStateTransform(const BiquadCascade6& a, const BiquadCascade6& b, float threshold = 1e-6f)
    {
        for (int i = 0; i < 6; ++i)
        {
            if (needsStateTransform(a.s[i], b.s[i], threshold))
                return true;
        }
        return false;
    }
}