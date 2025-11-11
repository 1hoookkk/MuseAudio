#pragma once
#include <cmath>
#include <algorithm>

namespace engine::ui
{
    /**
     * Overdamped spring for smooth, organic UI animations.
     * No oscillation, always settles smoothly to target.
     *
     * Usage:
     *   SpringValue<float> x(0.2f); // 200ms settle time
     *   x.setTarget(1.0f);
     *   x.update(deltaTime); // call at ~60fps
     *   float current = x.getValue();
     */
    template <typename T>
    class SpringValue
    {
    public:
        explicit SpringValue(float settleTimeSeconds = 0.2f) noexcept
            : settleTime(std::max(0.001f, settleTimeSeconds))
        {
            // Overdamped spring: zeta > 1
            // For zeta=2 (critically damped), response time ≈ 4/omega
            omega = 4.0f / settleTime;
            zeta = 2.0f; // critically damped
        }

        void setTarget(T newTarget) noexcept { target = newTarget; }
        void setCurrent(T newCurrent) noexcept { current = newCurrent; velocity = T{}; }
        void setCurrentAndTarget(T value) noexcept { current = target = value; velocity = T{}; }

        T getValue() const noexcept { return current; }
        T getTarget() const noexcept { return target; }

        void update(float dt) noexcept
        {
            if (dt <= 0.0f || dt > 1.0f) return; // sanity check

            // Spring-damper ODE: x'' + 2*zeta*omega*x' + omega^2*x = omega^2*target
            const T displacement = target - current;
            const T springForce = displacement * (omega * omega);
            const T dampingForce = velocity * (-2.0f * zeta * omega);
            const T acceleration = springForce + dampingForce;

            // Semi-implicit Euler (stable for springs)
            velocity = velocity + acceleration * dt;
            current = current + velocity * dt;

            // Snap to target if very close (avoid endless tiny movements)
            if (std::abs(displacement) < T{0.0001f} && std::abs(velocity) < T{0.001f})
            {
                current = target;
                velocity = T{};
            }
        }

    private:
        T current{};
        T target{};
        T velocity{};
        float omega;     // natural frequency
        float zeta;      // damping ratio
        float settleTime;
    };

    // Specialization for types that need different abs() - just use the template default for now
}
