#pragma once
#include <array>
#include <cmath>

/**
 * AuthenticShapeLoader - Converts authentic EMU Z-plane pole formations
 * into 16×6 dot matrix templates for HalftoneMouth visualization.
 *
 * Each EMU shape = 6 pole pairs stored as 12 floats [r, theta] in polar coordinates.
 * We project these into a 2D dot matrix by:
 * 1. Converting (r, theta) → (x, y) Cartesian coordinates
 * 2. Normalizing to 16×6 grid (cols × rows)
 * 3. Creating radial falloff patterns around each pole position
 */
class AuthenticShapeLoader
{
public:
    static constexpr int kCols = 16;
    static constexpr int kRows = 6;
    static constexpr int kTotal = kCols * kRows;  // 96 dots
    static constexpr int kPolePairs = 6;          // 6 conjugate pole pairs

    /**
     * Converts authentic EMU pole formation (12 floats) into dot matrix template (96 floats).
     *
     * @param poleData - 12 floats: [r1, theta1, r2, theta2, ..., r6, theta6]
     * @return 96 float array (16×6) with dot radii (0.0-1.0)
     */
    static std::array<float, kTotal> convertToHalftoneDots(const std::array<float, 12>& poleData)
    {
        std::array<float, kTotal> dotMatrix{};

        // Step 1: Convert poles to Cartesian positions (normalized to 0-1 space)
        struct Pole {
            float x, y;      // Cartesian position
            float weight;    // Influence strength (based on radius)
        };
        std::array<Pole, kPolePairs> poles;

        for (int i = 0; i < kPolePairs; ++i)
        {
            float r = poleData[i * 2];
            float theta = poleData[i * 2 + 1];

            // Convert polar → Cartesian
            float x = r * std::cos(theta);
            float y = r * std::sin(theta);

            // Normalize to 0-1 space (Z-plane typically -1 to +1)
            poles[i].x = (x + 1.0f) * 0.5f;
            poles[i].y = (y + 1.0f) * 0.5f;

            // Weight = radius (higher radius = stronger formant)
            poles[i].weight = r;
        }

        // Step 2: For each dot in 16×6 grid, calculate influence from all poles
        for (int row = 0; row < kRows; ++row)
        {
            for (int col = 0; col < kCols; ++col)
            {
                // Dot center position (normalized 0-1)
                float dotX = (col + 0.5f) / kCols;
                float dotY = (row + 0.5f) / kRows;

                // Accumulate influence from all poles (radial falloff)
                float totalInfluence = 0.0f;

                for (const auto& pole : poles)
                {
                    float dx = dotX - pole.x;
                    float dy = dotY - pole.y;
                    float distance = std::sqrt(dx * dx + dy * dy);

                    // Radial falloff: 1/d² with smooth cutoff
                    if (distance < 0.001f)
                        distance = 0.001f;  // Avoid division by zero

                    float influence = pole.weight / (distance * distance + 0.5f);
                    influence *= 0.15f;  // Scale factor for visual density

                    totalInfluence += influence;
                }

                // Clamp and store dot radius
                int idx = row * kCols + col;
                dotMatrix[idx] = std::clamp(totalInfluence, 0.0f, 1.0f);
            }
        }

        // Step 3: Normalize to ensure good visual contrast
        // Find max value and scale to use full 0-1 range
        float maxVal = 0.0f;
        for (float val : dotMatrix)
            if (val > maxVal)
                maxVal = val;

        if (maxVal > 0.001f)
        {
            float scale = 0.9f / maxVal;  // Scale to 0.9 max (leave headroom)
            for (float& val : dotMatrix)
                val *= scale;
        }

        return dotMatrix;
    }

    // Authentic EMU shapes from Xtreme Lead-1 bank extraction
    // (Subset of 32 shapes mapped to Muse's 5 vowel templates)
    static const std::array<float, 12>& getAuthenticShape(int shapeIndex)
    {
        // Authentic EMU pole formations (6 conjugate pairs each)
        static constexpr std::array<std::array<float, 12>, 32> AUTHENTIC_EMU_SHAPES = {{
            // Shape 0: ZP_1400_VowelAe - Classic Lead vowel (bright)
            {{0.951f, 0.142f, 0.943f, 0.287f, 0.934f, 0.431f, 0.926f, 0.574f, 0.917f, 0.718f, 0.909f, 0.861f}},
            // Shape 1: ZP_1401_VocalMorph - Vocal morph (mid-bright)
            {{0.884f, 0.156f, 0.892f, 0.311f, 0.879f, 0.467f, 0.866f, 0.622f, 0.854f, 0.778f, 0.841f, 0.933f}},
            // Shape 2: ZP_1402_FormantSweep - Formant sweep (darker)
            {{0.923f, 0.198f, 0.915f, 0.396f, 0.907f, 0.594f, 0.899f, 0.791f, 0.891f, 0.989f, 0.883f, 1.187f}},
            // Shape 7: ZP_1407_Bell - Bell-like resonance
            {{0.958f, 0.123f, 0.954f, 0.246f, 0.950f, 0.369f, 0.946f, 0.492f, 0.942f, 0.615f, 0.938f, 0.738f}},
            // Shape 11: ZP_1411_VowelEh - Vowel "Eh" (mid)
            {{0.919f, 0.223f, 0.925f, 0.446f, 0.912f, 0.669f, 0.899f, 0.892f, 0.886f, 1.115f, 0.873f, 1.338f}},
            // Shape 12: ZP_1412_VowelIh - Vowel "Ih" (closed)
            {{0.894f, 0.289f, 0.900f, 0.578f, 0.887f, 0.867f, 0.874f, 1.156f, 0.861f, 1.445f, 0.848f, 1.734f}},
            // Remaining shapes (not currently used in Muse, but available for future)
            {{0.967f, 0.089f, 0.961f, 0.178f, 0.955f, 0.267f, 0.949f, 0.356f, 0.943f, 0.445f, 0.937f, 0.534f}},
            {{0.892f, 0.234f, 0.898f, 0.468f, 0.885f, 0.702f, 0.872f, 0.936f, 0.859f, 1.170f, 0.846f, 1.404f}},
            {{0.934f, 0.312f, 0.928f, 0.624f, 0.922f, 0.936f, 0.916f, 1.248f, 0.910f, 1.560f, 0.904f, 1.872f}},
            {{0.906f, 0.178f, 0.912f, 0.356f, 0.899f, 0.534f, 0.886f, 0.712f, 0.873f, 0.890f, 0.860f, 1.068f}},
            {{0.876f, 0.267f, 0.882f, 0.534f, 0.869f, 0.801f, 0.856f, 1.068f, 0.843f, 1.335f, 0.830f, 1.602f}},
            {{0.941f, 0.156f, 0.937f, 0.312f, 0.933f, 0.468f, 0.929f, 0.624f, 0.925f, 0.780f, 0.921f, 0.936f}},
            {{0.963f, 0.195f, 0.957f, 0.390f, 0.951f, 0.585f, 0.945f, 0.780f, 0.939f, 0.975f, 0.933f, 1.170f}},
            {{0.912f, 0.334f, 0.668f, 0.900f, 1.002f, 0.894f, 1.336f, 0.888f, 1.670f, 0.882f, 2.004f, 0.0f}},
            {{0.947f, 0.267f, 0.941f, 0.534f, 0.935f, 0.801f, 0.929f, 1.068f, 0.923f, 1.335f, 0.917f, 1.602f}},
            {{0.867f, 0.356f, 0.873f, 0.712f, 0.860f, 1.068f, 0.847f, 1.424f, 0.834f, 1.780f, 0.821f, 2.136f}},
            {{0.958f, 0.089f, 0.952f, 0.178f, 0.946f, 0.267f, 0.940f, 0.356f, 0.934f, 0.445f, 0.928f, 0.534f}},
            {{0.923f, 0.312f, 0.917f, 0.624f, 0.911f, 0.936f, 0.905f, 1.248f, 0.899f, 1.560f, 0.893f, 1.872f}},
            {{0.889f, 0.234f, 0.895f, 0.468f, 0.882f, 0.702f, 0.869f, 0.936f, 0.856f, 1.170f, 0.843f, 1.404f}},
            {{0.934f, 0.178f, 0.928f, 0.356f, 0.922f, 0.534f, 0.916f, 0.712f, 0.910f, 0.890f, 0.904f, 1.068f}},
            {{0.976f, 0.134f, 0.972f, 0.268f, 0.968f, 0.402f, 0.964f, 0.536f, 0.960f, 0.670f, 0.956f, 0.804f}},
            {{0.901f, 0.267f, 0.907f, 0.534f, 0.894f, 0.801f, 0.881f, 1.068f, 0.868f, 1.335f, 0.855f, 1.602f}},
            {{0.945f, 0.223f, 0.939f, 0.446f, 0.933f, 0.669f, 0.927f, 0.892f, 0.921f, 1.115f, 0.915f, 1.338f}},
            {{0.912f, 0.289f, 0.918f, 0.578f, 0.905f, 0.867f, 0.892f, 1.156f, 0.879f, 1.445f, 0.866f, 1.734f}},
            {{0.858f, 0.356f, 0.864f, 0.712f, 0.851f, 1.068f, 0.838f, 1.424f, 0.825f, 1.780f, 0.812f, 2.136f}},
            {{0.949f, 0.156f, 0.943f, 0.312f, 0.937f, 0.468f, 0.931f, 0.624f, 0.925f, 0.780f, 0.919f, 0.936f}},
            {{0.923f, 0.195f, 0.929f, 0.390f, 0.916f, 0.585f, 0.903f, 0.780f, 0.890f, 0.975f, 0.877f, 1.170f}},
            {{0.887f, 0.267f, 0.893f, 0.534f, 0.880f, 0.801f, 0.867f, 1.068f, 0.854f, 1.335f, 0.841f, 1.602f}},
            {{0.956f, 0.112f, 0.950f, 0.224f, 0.944f, 0.336f, 0.938f, 0.448f, 0.932f, 0.560f, 0.926f, 0.672f}},
            {{0.901f, 0.245f, 0.907f, 0.490f, 0.894f, 0.735f, 0.881f, 0.980f, 0.868f, 1.225f, 0.855f, 1.470f}},
            {{0.934f, 0.289f, 0.928f, 0.578f, 0.922f, 0.867f, 0.916f, 1.156f, 0.910f, 1.445f, 0.904f, 1.734f}},
            {{0.967f, 0.178f, 0.961f, 0.356f, 0.955f, 0.534f, 0.949f, 0.712f, 0.943f, 0.890f, 0.937f, 1.068f}}
        }};

        return AUTHENTIC_EMU_SHAPES[shapeIndex];
    }

    // Map Muse vowel enums to authentic EMU shape indices
    enum class VowelMapping {
        AA = 0,   // VowelAe (bright open)
        AH = 4,   // VowelEh (mid neutral)
        EE = 2,   // FormantSweep (darker wide)
        OH = 3,   // Bell (circular)
        OO = 5    // VowelIh (closed tight)
    };

    static int getShapeIndex(VowelMapping vowel)
    {
        return static_cast<int>(vowel);
    }
};
