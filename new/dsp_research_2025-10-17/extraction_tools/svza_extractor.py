#!/usr/bin/env python3
"""
SVZa Zenology Bank Extractor
Decompresses SVZa files and extracts EMU Audity coefficient data
Usage: python svza_extractor.py <file.bin> [--output results/]
"""

import sys
import os
import zlib
import struct
import json
from pathlib import Path

def extract_svza(filepath, output_dir="results"):
    """Extract and decompress SVZa Zenology bank file"""

    with open(filepath, 'rb') as f:
        data = f.read()

    # Parse SVZa header
    if data[:4] != b'SVZa':
        raise ValueError("Not a valid SVZa file")

    print(f"File: {filepath}")
    print(f"Size: {len(data)} bytes")
    print(f"Magic: {data[:4]}")

    # Find compressed data (starts with zlib header 0x789c)
    zlib_start = None
    for i in range(len(data) - 1):
        if data[i:i+2] == b'\x78\x9c':
            zlib_start = i
            break

    if zlib_start is None:
        raise ValueError("No zlib compressed data found")

    print(f"Compressed data starts at offset: 0x{zlib_start:04x}")

    # Extract and decompress
    compressed_data = data[zlib_start:]
    try:
        decompressed = zlib.decompress(compressed_data)
        print(f"Decompressed size: {len(decompressed)} bytes")
    except zlib.error as e:
        raise ValueError(f"Decompression failed: {e}")

    # Create output directory
    output_path = Path(output_dir)
    output_path.mkdir(exist_ok=True)

    # Save decompressed data
    basename = Path(filepath).stem
    raw_file = output_path / f"{basename}_decompressed.bin"
    with open(raw_file, 'wb') as f:
        f.write(decompressed)
    print(f"Saved raw decompressed data: {raw_file}")

    # Analyze decompressed content
    analyze_decompressed_data(decompressed, output_path, basename)

    return decompressed

def analyze_decompressed_data(data, output_path, basename):
    """Analyze decompressed Zenology data for EMU content"""

    print("\nAnalyzing decompressed data...")

    # Look for ASCII strings (preset names, metadata)
    strings = find_ascii_strings(data)
    if strings:
        strings_file = output_path / f"{basename}_strings.txt"
        with open(strings_file, 'w') as f:
            for s in strings:
                f.write(f"{s}\n")
        print(f"Found {len(strings)} ASCII strings, saved to {strings_file}")

        # Print first few strings
        print("Sample strings:")
        for s in strings[:20]:
            print(f"  '{s}'")

    # Look for float32 coefficient data
    try:
        import numpy as np

        # Try different alignments for float32 data
        for offset in [0, 4, 8, 16]:
            if offset >= len(data):
                continue

            aligned_data = data[offset:]
            if len(aligned_data) % 4 != 0:
                aligned_data = aligned_data[:-(len(aligned_data) % 4)]

            if len(aligned_data) < 16:
                continue

            floats = np.frombuffer(aligned_data, dtype='<f4')

            # Find reasonable coefficient ranges (-10 to +10 typical for filter coeffs)
            valid_mask = (np.abs(floats) <= 10.0) & np.isfinite(floats)

            if np.sum(valid_mask) > 100:  # At least 100 valid coefficients
                valid_floats = floats[valid_mask]
                coeffs_file = output_path / f"{basename}_coefficients_offset{offset}.csv"
                np.savetxt(coeffs_file, valid_floats, fmt='%.9g')
                print(f"Found {len(valid_floats)} potential coefficients at offset {offset}")
                print(f"  Range: {np.min(valid_floats):.6f} to {np.max(valid_floats):.6f}")
                print(f"  Saved to: {coeffs_file}")

                # Look for Z-plane signature patterns
                if check_zplane_patterns(valid_floats):
                    print(f"  *** POTENTIAL Z-PLANE DATA DETECTED ***")

    except ImportError:
        print("NumPy not available, skipping coefficient analysis")

    # Look for specific EMU signatures
    check_emu_signatures(data, basename)

def find_ascii_strings(data, minlen=6, maxlen=200):
    """Extract ASCII strings from binary data"""
    strings = []
    current = []

    for b in data:
        if 32 <= b <= 126:  # Printable ASCII
            current.append(chr(b))
        else:
            if minlen <= len(current) <= maxlen:
                strings.append(''.join(current))
            current = []

    # Don't forget the last string
    if minlen <= len(current) <= maxlen:
        strings.append(''.join(current))

    return strings

def check_zplane_patterns(floats):
    """Check for Z-plane filter coefficient patterns"""
    import numpy as np

    # Z-plane filters typically have:
    # 1. Coefficients in reasonable ranges
    # 2. Some periodicity (repeated sections)
    # 3. Specific pole/zero relationships

    # Check for coefficient groupings (biquad sections = 5 coeffs each)
    if len(floats) % 5 == 0:
        print("    - Data length is multiple of 5 (biquad sections)")
        return True

    # Check for coefficient groupings (6-section filters = 30 coeffs)
    if len(floats) % 30 == 0:
        print("    - Data length is multiple of 30 (6-section biquads)")
        return True

    # Check for typical Z-plane coefficient ranges
    in_zplane_range = np.sum((np.abs(floats) >= 0.001) & (np.abs(floats) <= 2.0))
    if in_zplane_range / len(floats) > 0.8:
        print(f"    - {in_zplane_range/len(floats)*100:.1f}% coefficients in Z-plane range")
        return True

    return False

def check_emu_signatures(data, basename):
    """Look for specific EMU/Audity signatures in the data"""

    # Common EMU strings
    emu_markers = [b'EMU', b'Audity', b'AUDITY', b'Z-Plane', b'ZPLANE',
                   b'Proteus', b'PROTEUS', b'Morpheus', b'MORPHEUS',
                   b'Vintage', b'VINTAGE', b'Keys', b'KEYS']

    found_markers = []
    for marker in emu_markers:
        if marker in data:
            offset = data.find(marker)
            found_markers.append((marker.decode('ascii', errors='ignore'), offset))

    if found_markers:
        print(f"\nFound EMU signatures:")
        for marker, offset in found_markers:
            print(f"  '{marker}' at offset 0x{offset:04x}")
        return True

    return False

def main():
    if len(sys.argv) < 2:
        print("Usage: python svza_extractor.py <file.bin> [--output results/]")
        sys.exit(1)

    filepath = sys.argv[1]
    output_dir = "results"

    if "--output" in sys.argv:
        idx = sys.argv.index("--output")
        if idx + 1 < len(sys.argv):
            output_dir = sys.argv[idx + 1]

    try:
        extract_svza(filepath, output_dir)
        print(f"\nExtraction complete! Results saved in '{output_dir}/'")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()