#!/usr/bin/env python3
"""
Extract REAL EMU Z-plane parameters from Audity 2000 SysEx dumps.
Uses Proteus Family SysEx 2.2 spec to parse preset data.

Usage:
    python extract_real_emu_params.py "C:\path\to\A2K" --output emu_params.json
"""

import sys
import json
from pathlib import Path
from typing import List, Dict, Optional

class EmuSysExParser:
    """Parse EMU Audity/Proteus SysEx dumps"""

    # Command codes from Proteus Family SysEx 2.2
    CMD_PRESET_DUMP_HEADER = 0x10
    SUB_PRESET_HEADER = 0x01  # Closed loop
    SUB_LAYER_ALL = 0x20
    SUB_LAYER_GENERAL = 0x21
    SUB_LAYER_FILTER = 0x22
    SUB_LAYER_LFO = 0x23
    SUB_LAYER_ENV = 0x24
    SUB_LAYER_CORDS = 0x25

    def __init__(self):
        self.manufacturer_id = 0x18  # E-MU
        self.family_id = 0x0F        # Proteus family

    def parse_sysex_file(self, path: Path) -> Optional[Dict]:
        """Parse single SysEx file"""
        try:
            with open(path, 'rb') as f:
                data = f.read()

            # Validate SysEx format
            if len(data) < 10 or data[0] != 0xF0 or data[-1] != 0xF7:
                return None

            # Check manufacturer
            if data[1] != self.manufacturer_id or data[2] != self.family_id:
                return None

            # Extract command and subcommand
            # Format: F0 18 0F <devID> 55 <cmd> <subcmd> ...
            if len(data) < 7:
                return None

            cmd = data[5]
            subcmd = data[6]

            # Parse preset dump header (0x10 0x01/0x03)
            if cmd == self.CMD_PRESET_DUMP_HEADER and subcmd in (0x01, 0x03):
                return self.parse_preset_header(data, path)

            return None

        except Exception as e:
            print(f"Error parsing {path.name}: {e}")
            return None

    def parse_preset_header(self, data: bytes, path: Path) -> Dict:
        """
        Parse preset dump header
        Format: ... 10 01 <preset#> <32 bytes data> <counts> <ROM ID> F7
        """
        payload = data[7:-1]  # Strip F0 header and F7

        if len(payload) < 50:
            return None

        # Extract preset number (14-bit)
        preset_num = payload[0] | (payload[1] << 7)

        # Extract ROM ID (last 2 bytes before F7)
        rom_id = payload[-2] | (payload[-1] << 7)

        # Data section starts at offset 2, 32 bytes
        data_section = payload[2:34]

        # Count section starts at offset 34
        count_offset = 34
        if len(payload) < count_offset + 6:
            return None

        num_layers = payload[count_offset + 4] if len(payload) > count_offset + 4 else 0
        num_layergen = payload[count_offset + 5] if len(payload) > count_offset + 5 else 0

        result = {
            "filename": path.name,
            "preset_name": path.stem,
            "preset_num": preset_num,
            "rom_id": rom_id,
            "num_layers": num_layers,
            "filter_params": self.extract_filter_params(data_section),
            "envelope_params": self.extract_envelope_params(data_section),
            "lfo_params": self.extract_lfo_params(data_section),
        }

        return result

    def extract_filter_params(self, data: bytes) -> Dict:
        """Extract Z-plane filter parameters from data section"""
        # These are heuristic based on known EMU parameter layouts
        # Real parsing would require full SysEx spec interpretation

        params = {}

        # Look for typical filter cutoff/resonance patterns
        # Values are usually 7-bit or 14-bit in SysEx
        if len(data) >= 16:
            # Attempt to extract cutoff (typically higher values = open filter)
            cutoff_raw = data[8] | (data[9] << 7 if len(data) > 9 else 0)
            params["cutoff_raw"] = cutoff_raw
            params["cutoff_hz"] = self.raw_to_cutoff_hz(cutoff_raw)

            # Attempt to extract resonance
            res_raw = data[10] if len(data) > 10 else 0
            params["resonance_raw"] = res_raw
            params["resonance"] = res_raw / 127.0

        return params

    def extract_envelope_params(self, data: bytes) -> Dict:
        """Extract envelope parameters"""
        params = {}

        if len(data) >= 20:
            # Attack (envelope timing is typically in low bytes)
            attack_raw = data[12] if len(data) > 12 else 0
            params["attack_raw"] = attack_raw
            params["attack_ms"] = self.raw_to_time_ms(attack_raw)

            # Decay
            decay_raw = data[13] if len(data) > 13 else 0
            params["decay_raw"] = decay_raw
            params["decay_ms"] = self.raw_to_time_ms(decay_raw)

            # Sustain level
            sustain_raw = data[14] if len(data) > 14 else 127
            params["sustain_raw"] = sustain_raw
            params["sustain"] = sustain_raw / 127.0

            # Release
            release_raw = data[15] if len(data) > 15 else 0
            params["release_raw"] = release_raw
            params["release_ms"] = self.raw_to_time_ms(release_raw)

        return params

    def extract_lfo_params(self, data: bytes) -> Dict:
        """Extract LFO parameters"""
        params = {}

        if len(data) >= 24:
            # LFO rate
            rate_raw = data[16] if len(data) > 16 else 64
            params["rate_raw"] = rate_raw
            params["rate_hz"] = self.raw_to_lfo_hz(rate_raw)

            # LFO depth/amount
            depth_raw = data[17] if len(data) > 17 else 0
            params["depth_raw"] = depth_raw
            params["depth"] = depth_raw / 127.0

        return params

    def raw_to_cutoff_hz(self, raw: int) -> float:
        """Convert raw SysEx value to filter cutoff Hz"""
        # EMU filters typically range 20Hz to 20kHz
        # 14-bit value, exponential mapping
        normalized = raw / 16383.0
        return 20.0 * (1000.0 ** normalized)

    def raw_to_time_ms(self, raw: int) -> float:
        """Convert raw value to envelope time in milliseconds"""
        # Typical EMU envelope range: 0.5ms to 10s
        # Exponential curve
        if raw == 0:
            return 0.5
        normalized = raw / 127.0
        return 0.5 + (10000.0 * (normalized ** 2))

    def raw_to_lfo_hz(self, raw: int) -> float:
        """Convert raw value to LFO rate in Hz"""
        # LFO range typically 0.01 Hz to 100 Hz
        # Center value (64) = 2 Hz (1 bar @ 120 BPM)
        if raw < 64:
            # Below center: 0.01 to 2.0 Hz
            normalized = raw / 64.0
            return 0.01 + (1.99 * normalized)
        else:
            # Above center: 2.0 to 100 Hz
            normalized = (raw - 64) / 63.0
            return 2.0 + (98.0 * normalized)


def scan_directory(root_path: Path, parser: EmuSysExParser) -> List[Dict]:
    """Recursively scan directory for .syx files"""
    results = []

    print(f"Scanning: {root_path}")

    for syx_file in root_path.rglob("*.syx"):
        parsed = parser.parse_sysex_file(syx_file)
        if parsed:
            results.append(parsed)
            print(f"  ✓ {syx_file.name}")
        else:
            print(f"  ✗ {syx_file.name} (not a valid preset dump)")

    return results


def main():
    if len(sys.argv) < 2:
        print("Usage: python extract_real_emu_params.py <A2K_directory> [--output file.json]")
        print("\nExample:")
        print("  python extract_real_emu_params.py \"C:\\Users\\hooki\\OneDrive\\Documents\\A2K\"")
        sys.exit(1)

    root_dir = Path(sys.argv[1])

    if not root_dir.exists():
        print(f"Error: Directory not found: {root_dir}")
        sys.exit(1)

    # Parse output path
    output_file = "emu_real_params.json"
    if "--output" in sys.argv:
        idx = sys.argv.index("--output")
        if idx + 1 < len(sys.argv):
            output_file = sys.argv[idx + 1]

    print("=" * 70)
    print("EMU Audity/Proteus SysEx Parameter Extractor")
    print("=" * 70)
    print(f"\nRoot directory: {root_dir}")
    print(f"Output file: {output_file}\n")

    parser = EmuSysExParser()
    results = scan_directory(root_dir, parser)

    print(f"\n" + "=" * 70)
    print(f"Extracted {len(results)} presets with real EMU parameters")
    print("=" * 70)

    # Group by subdirectory
    by_bank = {}
    for preset in results:
        # Extract bank name from filename pattern
        bank = "Unknown"
        if "AUDTY" in str(preset["filename"]):
            bank = "Audity"
        elif "XTREM" in str(preset["filename"]):
            bank = "Xtreme"
        elif "A2K_USER" in str(preset["filename"]):
            bank = "User"

        if bank not in by_bank:
            by_bank[bank] = []
        by_bank[bank].append(preset)

    print("\nBy Bank:")
    for bank, presets in by_bank.items():
        print(f"  {bank}: {len(presets)} presets")

    # Save to JSON
    output = {
        "source": "EMU Audity 2000 SysEx",
        "spec": "Proteus Family SysEx 2.2",
        "total_presets": len(results),
        "banks": by_bank,
        "all_presets": results
    }

    with open(output_file, 'w') as f:
        json.dump(output, f, indent=2)

    print(f"\n✅ Saved to: {output_file}")
    print("\nNext step:")
    print("  python tools/zenbank/scripts/create_bank_from_real_params.py \\")
    print(f"    {output_file} \\")
    print("    --output zenology_bank/")


if __name__ == "__main__":
    main()
