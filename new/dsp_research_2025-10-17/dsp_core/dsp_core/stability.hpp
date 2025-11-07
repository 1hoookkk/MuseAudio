#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>
#include <algorithm>

namespace dsp
{

inline void assertMonic (const std::vector<double>& a)
{
    if (a.empty() || std::abs (a[0] - 1.0) > 1.0e-12)
        throw std::runtime_error ("Denominator must be monic (a0 == 1)");
}

inline std::vector<double> stepDownToReflection (std::vector<double> a)
{
    assertMonic (a);
    const int N = static_cast<int> (a.size()) - 1;
    std::vector<double> k (N + 1, 0.0);
    std::vector<double> current = a;

    for (int m = N; m >= 1; --m)
    {
        double km = -current[m]; // reflection coefficient
        if (std::abs (km) >= 1.0)
            km = std::copysign (0.999999, km);
        k[m] = km;
        if (m == 1)
            break;

        std::vector<double> prev (m, 0.0);
        const double denom = 1.0 - km * km;
        for (int i = 1; i <= m - 1; ++i)
            prev[i] = (current[i] + km * current[m - i]) / denom;

        current.resize (m);
        for (int i = 1; i <= m - 1; ++i)
            current[i] = prev[i];
    }
    return k;
}

inline std::vector<double> stepUpFromReflection (const std::vector<double>& k)
{
    const int N = static_cast<int> (k.size()) - 1;
    std::vector<double> a (2, 0.0);
    a[0] = 1.0;
    a[1] = -k[1];
    for (int m = 2; m <= N; ++m)
    {
        std::vector<double> next (m + 1, 0.0);
        next[0] = 1.0;
        for (int i = 1; i <= m - 1; ++i)
            next[i] = a[i] - k[m] * a[m - i];
        next[m] = -k[m];
        a = std::move (next);
    }
    return a;
}

inline void stabilizeDen (std::vector<double>& a, double kMax = 0.995)
{
    auto k = stepDownToReflection (a);
    for (size_t i = 1; i < k.size(); ++i)
        k[i] = std::clamp (k[i], -kMax, kMax);
    a = stepUpFromReflection (k);
}

inline void stabilizeSOS (double& a1, double& a2, double kMax = 0.995)
{
    std::vector<double> a { 1.0, a1, a2 };
    stabilizeDen (a, kMax);
    a1 = a[1];
    a2 = a[2];
}

} // namespace dsp
