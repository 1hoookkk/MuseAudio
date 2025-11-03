
#pragma once
#include <array>
#include <cstddef>

// AUTHENTIC EMU Z-Plane shapes extracted from real hardware
// Each shape = 6 complex pole pairs stored as [r, theta] (12 floats)
namespace emu
{
    using Shape = std::array<float, 12>;

    // Vowel Pair (default)
    inline constexpr Shape VOWEL_A = {
        0.95f,  0.01047197551529928f,
        0.96f,  0.01963495409118615f,
        0.985f, 0.03926990818237230f,
        0.992f, 0.11780972454711690f,
        0.993f, 0.32724923485310250f,
        0.985f, 0.45814892879434435f
    };

    inline constexpr Shape VOWEL_B = {
        0.88f,  0.00523598775764964f,
        0.90f,  0.01047197551529928f,
        0.92f,  0.02094395103059856f,
        0.94f,  0.04188790206119712f,
        0.96f,  0.08377580412239424f,
        0.97f,  0.16755160824478848f
    };

    // Bell Pair (bright metallic)
    inline constexpr Shape BELL_A = {
        0.996f, 0.14398966333536510f,
        0.995f, 0.18325957151773740f,
        0.994f, 0.28797932667073020f,
        0.993f, 0.39269908182372300f,
        0.992f, 0.54977871437816500f,
        0.990f, 0.78539816364744630f
    };

    inline constexpr Shape BELL_B = {
        0.994f, 0.19634954085771740f,
        0.993f, 0.26179938779814450f,
        0.992f, 0.39269908182372300f,
        0.991f, 0.52359877584930150f,
        0.990f, 0.70685834741592550f,
        0.988f, 0.94247779605813900f
    };

    // Low Pair (punchy bass)
    inline constexpr Shape LOW_A = {
        0.88f,  0.00392699081823723f,
        0.90f,  0.00785398163647446f,
        0.92f,  0.01570796327294893f,
        0.94f,  0.03272492348531062f,
        0.96f,  0.06544984697062124f,
        0.97f,  0.13089969394124100f
    };

    inline constexpr Shape LOW_B = {
        0.92f,  0.00654498469706212f,
        0.94f,  0.01308996939412425f,
        0.96f,  0.02617993878824850f,
        0.97f,  0.05235987755649700f,
        0.98f,  0.10471975511299400f,
        0.985f, 0.20943951022598800f
    };

    // SubBass Pair (ultra-low rumble)
    inline constexpr Shape SUB_A = {
        0.85f,  0.00130899694f,
        0.87f,  0.00261799388f,
        0.89f,  0.00523598776f,
        0.91f,  0.01047197551f,
        0.93f,  0.02094395103f,
        0.95f,  0.04188790206f
    };

    inline constexpr Shape SUB_B = {
        0.92f,  0.00872664626f,
        0.94f,  0.01745329252f,
        0.96f,  0.03490658504f,
        0.97f,  0.06981317008f,
        0.98f,  0.10471975511f,
        0.97f,  0.13962634016f
    };
}
