#!/usr/bin/env python3
"""
Convert Z-plane poles from 48kHz to 44.1kHz using mathematical transformation.

Given poles at Fs_src = 48000 Hz, derive poles at Fs_dst = 44100 Hz:
- Map to continuous time: s = Fs_src * (ln(r) + jθ)
- Map to new rate: z' = exp(s * T_dst)
  - r' = r^(Fs_dst/Fs_src)
  - θ' = θ * (Fs_dst/Fs_src)
- Wrap θ' to (-π, π] and clamp r' just below 1.0 if needed
"""

import json
import math
from pathlib import Path


def wrap_angle(theta):
    """Wrap angle to (-π, π]"""
    while theta > math.pi:
        theta -= 2 * math.pi
    while theta <= -math.pi:
        theta += 2 * math.pi
    return theta


def convert_pole(r_src, theta_src, fs_ratio):
    """
    Convert pole from source sample rate to destination sample rate.

    Args:
        r_src: Pole radius at source sample rate
        theta_src: Pole angle at source sample rate
        fs_ratio: Fs_dst / Fs_src (e.g., 44100/48000 = 0.91875)

    Returns:
        (r_dst, theta_dst): Converted pole
    """
    # Power law for radius (exponential of log-space scaling)
    r_dst = r_src ** fs_ratio

    # Linear scaling for angle
    theta_dst = theta_src * fs_ratio

    # Wrap angle to (-π, π]
    theta_dst = wrap_angle(theta_dst)

    # Clamp radius for stability (max 0.9999)
    r_dst = min(r_dst, 0.9999)

    return r_dst, theta_dst


def convert_shapes_file(input_path, output_path, fs_src, fs_dst):
    """Convert entire shapes JSON file to new sample rate"""

    with open(input_path, 'r') as f:
        data = json.load(f)

    fs_ratio = fs_dst / fs_src

    # Update sample rate reference
    data['sampleRateRef'] = fs_dst

    # Convert all poles in all shapes
    for shape in data['shapes']:
        new_poles = []
        for pole in shape['poles']:
            r_src = pole['r']
            theta_src = pole['theta']
            r_dst, theta_dst = convert_pole(r_src, theta_src, fs_ratio)
            new_poles.append({
                'r': r_dst,
                'theta': theta_dst
            })
        shape['poles'] = new_poles

    # Write output
    with open(output_path, 'w') as f:
        json.dump(data, f, indent=2)

    return data


def main():
    """Convert both A and B shape files from 48kHz to 44.1kHz"""

    shapes_dir = Path(__file__).parent

    fs_src = 48000
    fs_dst = 44100

    print(f"Converting Z-plane shapes: {fs_src} Hz → {fs_dst} Hz")
    print(f"Sample rate ratio: {fs_dst/fs_src:.6f}")
    print()

    # Convert A shapes
    input_a = shapes_dir / 'audity_shapes_A_48k.json'
    output_a = shapes_dir / 'audity_shapes_A_44k.json'

    print(f"Converting: {input_a.name} → {output_a.name}")
    data_a = convert_shapes_file(input_a, output_a, fs_src, fs_dst)
    print(f"  ✓ Converted {len(data_a['shapes'])} shapes")

    # Convert B shapes
    input_b = shapes_dir / 'audity_shapes_B_48k.json'
    output_b = shapes_dir / 'audity_shapes_B_44k.json'

    print(f"Converting: {input_b.name} → {output_b.name}")
    data_b = convert_shapes_file(input_b, output_b, fs_src, fs_dst)
    print(f"  ✓ Converted {len(data_b['shapes'])} shapes")

    print()
    print("✅ Conversion complete!")
    print()
    print("Sample conversions:")
    for i, shape in enumerate(data_a['shapes'][:1]):  # Show first shape as example
        print(f"\n{shape['name']}:")
        print("  48kHz → 44.1kHz")

        # Load original for comparison
        with open(input_a) as f:
            orig = json.load(f)

        for j, (orig_pole, new_pole) in enumerate(zip(orig['shapes'][i]['poles'], shape['poles']), 1):
            r_change = ((new_pole['r'] / orig_pole['r']) - 1) * 100
            th_change = ((new_pole['theta'] / orig_pole['theta']) - 1) * 100
            print(f"  Pole {j}: r={orig_pole['r']:.6f}→{new_pole['r']:.6f} ({r_change:+.2f}%), "
                  f"θ={orig_pole['theta']:.6f}→{new_pole['theta']:.6f} ({th_change:+.2f}%)")


if __name__ == '__main__':
    main()
