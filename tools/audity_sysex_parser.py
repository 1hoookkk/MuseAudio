#!/usr/bin/env python3
"""
Audity 2000 SysEx Preset Parser
Extracts Z-plane filter parameters from EMU Audity 2000 preset dumps.

Usage:
    python audity_sysex_parser.py AUDTY/00_0036_REZANATOR.syx
    python audity_sysex_parser.py AUDTY/*.syx --output audity_presets.json
"""

import sys
import json
import struct
from pathlib import Path
from typing import Dict, List, Optional


class AuditySysExParser:
    """Parse EMU Audity 2000 SysEx preset files."""
    
    # EMU SysEx header: F0 18 04 (Manufacturer ID: E-MU)
    EMU_SYSEX_HEADER = b'\xF0\x18\x04'
    
    # Common offsets (these are estimates - need to verify with EMU docs)
    # Z-plane filter parameters typically in the voice section
    FILTER_SECTION_OFFSET = 0x20  # Approximate location
    
    def __init__(self, filepath: Path):
        self.filepath = filepath
        self.preset_name = filepath.stem.split('_', 2)[-1]  # Extract from "00_0036_REZANATOR"
        self.data = self._read_file()
        
    def _read_file(self) -> bytes:
        """Read SysEx file."""
        with open(self.filepath, 'rb') as f:
            return f.read()
    
    def _verify_sysex(self) -> bool:
        """Verify this is a valid EMU SysEx file."""
        if not self.data.startswith(self.EMU_SYSEX_HEADER):
            return False
        if self.data[-1] != 0xF7:  # SysEx End byte
            return False
        return True
    
    def extract_filter_params(self) -> Optional[Dict]:
        """
        Extract Z-plane filter parameters from preset.
        
        Returns dict with:
            - morph: 0-255 (filter morph position)
            - resonance: 0-255 (filter resonance/Q)
            - cutoff: 0-255 (if separate from morph)
            - mix: 0-255 (dry/wet mix)
            - env_to_morph: -128 to +127 (envelope modulation depth)
            - lfo_to_morph: -128 to +127 (LFO modulation depth)
        """
        if not self._verify_sysex():
            return None
        
        # EMU SysEx structure (from reverse engineering):
        # Bytes 0-2: F0 18 04 (Header)
        # Byte 3: Device ID
        # Byte 4: Command (preset dump)
        # Bytes 5+: Preset data (7-bit encoded)
        
        # Extract 7-bit MIDI data (bytes are 0-127, MSB always 0)
        midi_data = self.data[5:-1]  # Strip header and footer
        
        # Z-plane filter parameters are typically in voice section
        # This is a HEURISTIC - actual offsets need EMU documentation
        params = {}
        
        try:
            # Common EMU synth parameter layout (approximate)
            # Adjust these offsets based on actual EMU Audity documentation
            
            # Filter frequency/cutoff (often byte 32-40 range)
            if len(midi_data) > 35:
                params['filter_freq'] = midi_data[35]
            
            # Filter resonance/Q (often near cutoff)
            if len(midi_data) > 36:
                params['filter_res'] = midi_data[36]
            
            # Z-plane morph position (EMU-specific, often byte 50-70 range)
            if len(midi_data) > 60:
                params['z_morph'] = midi_data[60]
            
            # Mix/amount (often later in parameter list)
            if len(midi_data) > 70:
                params['filter_mix'] = midi_data[70]
            
            # Envelope to filter (modulation section, often byte 80-120)
            if len(midi_data) > 85:
                # Convert from 7-bit unsigned to signed (-64 to +63)
                env_depth = midi_data[85]
                params['env_to_filter'] = env_depth - 64 if env_depth > 64 else env_depth
            
            # LFO to filter
            if len(midi_data) > 90:
                lfo_depth = midi_data[90]
                params['lfo_to_filter'] = lfo_depth - 64 if lfo_depth > 64 else lfo_depth
            
        except IndexError:
            return None
        
        return params
    
    def to_dict(self) -> Dict:
        """Convert preset to dictionary."""
        # Parse filename: "00_0036_REZANATOR.syx"
        parts = self.filepath.stem.split('_')
        bank = int(parts[0]) if len(parts) > 0 else 0
        number = int(parts[1]) if len(parts) > 1 else 0
        name = parts[2] if len(parts) > 2 else "Unknown"
        
        params = self.extract_filter_params()
        
        return {
            'name': name,
            'bank': bank,
            'number': number,
            'file': str(self.filepath.name),
            'size_bytes': len(self.data),
            'is_valid': self._verify_sysex(),
            'filter_params': params
        }


def parse_preset(filepath: Path) -> Dict:
    """Parse a single preset file."""
    parser = AuditySysExParser(filepath)
    return parser.to_dict()


def parse_directory(directory: Path, limit: int = None) -> List[Dict]:
    """Parse all presets in a directory."""
    presets = []
    syx_files = sorted(directory.glob('*.syx'))
    
    if limit:
        syx_files = syx_files[:limit]
    
    for filepath in syx_files:
        try:
            preset = parse_preset(filepath)
            presets.append(preset)
        except Exception as e:
            print(f"Error parsing {filepath.name}: {e}", file=sys.stderr)
    
    return presets


def main():
    """CLI interface."""
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    input_path = Path(sys.argv[1])
    
    if input_path.is_file():
        # Parse single file
        preset = parse_preset(input_path)
        print(json.dumps(preset, indent=2))
    
    elif input_path.is_dir():
        # Parse directory
        limit = int(sys.argv[2]) if len(sys.argv) > 2 else None
        presets = parse_directory(input_path, limit)
        print(json.dumps({'presets': presets}, indent=2))
    
    else:
        print(f"Error: {input_path} not found", file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
