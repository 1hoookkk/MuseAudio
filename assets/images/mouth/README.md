# Halftone Mouth Assets

## Required PNG Files (AI-Generated)

Generate these 5 core mouth shapes as **white halftone dots on transparent/black background**:

### Core Shapes (Required):
- `mouth_AA_wide.png` - Wide open vertical oval (darkest/lowest formant)
- `mouth_AH_neutral.png` - Medium open, relaxed (neutral mid position)
- `mouth_EE_smile.png` - Horizontal slit, smile-like (brightest/highest formant)
- `mouth_OH_round.png` - Round bell-like opening
- `mouth_OO_tight.png` - Small focused circle

### Optional Enhancements:
- `mouth_closed.png` - Barely visible dots, dormant/idle state
- `mouth_glitch.png` - Distorted/broken dots for struggle/meltdown states

## Specifications:

**Dimensions:** 240×90 pixels (16:6 aspect ratio)  
**Style:** Halftone dot matrix / LED pixel aesthetic  
**Colors:** White dots (#FFFFFF) on transparent or black (#000000) background  
**Dot Grid:** Approximately 16×6 grid of varying-sized dots  
**Aesthetic:** Brutalist, minimal, iconic  
**NO:** Teeth, realistic details, gradients, 3D effects

## AI Generation Prompt Template:

```
Minimalist halftone dot matrix mouth shape showing [VOWEL] position.
White dots on black background, LED display aesthetic, brutalist design.
Varying dot sizes create form through halftone pattern.
No teeth, no realistic details, abstract iconic shape.
Grid pattern of dots, 240×90 pixels, monochrome.
Style: vintage LED matrix, printer halftone, Ben-Day dots.
```

Replace [VOWEL] with:
- AA: "wide open vertical oval"
- AH: "medium relaxed opening" 
- EE: "horizontal slit smile"
- OH: "round bell opening"
- OO: "small tight circle"

## Usage:

These PNGs will be automatically loaded into JUCE BinaryData by CMake.
The `HalftoneMouth` component will crossfade between them based on DSP vowel state.
