//! Plugin integration example showing how to use in a JUCE/VST context

use dsp_plugin::{Filter, Oscillator, Waveform};

/// Simple plugin processor structure
struct AudioPlugin {
    filter: Filter,
    oscillator: Oscillator,
    sample_rate: f32,
}

impl AudioPlugin {
    /// Create new plugin instance
    fn new() -> Self {
        Self {
            filter: Filter::new(44100.0),
            oscillator: Oscillator::new(44100.0),
            sample_rate: 44100.0,
        }
    }

    /// Prepare for playback (called by host)
    fn prepare(&mut self, sample_rate: f32) {
        self.sample_rate = sample_rate;
        self.filter = Filter::new(sample_rate);
        self.oscillator = Oscillator::new(sample_rate);

        println!("Plugin prepared at {} Hz", sample_rate);
    }

    /// Process audio block (called by host)
    fn process_block(&mut self, buffer: &mut [f32]) {
        // Generate oscillator signal
        self.oscillator.process(buffer);

        // Filter the audio
        self.filter.process(buffer);
    }

    /// Set filter frequency (from UI parameter)
    fn set_filter_frequency(&mut self, freq: f32) {
        self.filter.set_frequency(freq);
    }

    /// Set filter resonance (from UI parameter)
    fn set_filter_resonance(&mut self, res: f32) {
        self.filter.set_resonance(res);
    }

    /// Set oscillator frequency (from MIDI note)
    fn set_note(&mut self, note: u8) {
        let freq = 440.0 * 2.0_f32.powf((note as f32 - 69.0) / 12.0);
        self.oscillator.set_frequency(freq);
    }

    /// Set oscillator waveform
    fn set_waveform(&mut self, waveform: Waveform) {
        self.oscillator.set_waveform(waveform);
    }
}

fn main() {
    println!("🎵 DSP Plugin Ready - Plugin Integration Example\n");

    // Create plugin instance
    let mut plugin = AudioPlugin::new();

    // Simulate host calling prepare
    plugin.prepare(48000.0);

    // Simulate parameter changes from UI
    plugin.set_filter_frequency(2000.0);
    plugin.set_filter_resonance(0.8);
    plugin.set_waveform(Waveform::Saw);

    // Simulate MIDI note-on
    plugin.set_note(60); // Middle C

    // Simulate audio processing
    let mut buffer = vec![0.0f32; 128];
    for block in 0..10 {
        plugin.process_block(&mut buffer);

        if block == 0 {
            println!(
                "First block RMS: {:.4}",
                (buffer.iter().map(|x| x * x).sum::<f32>() / buffer.len() as f32).sqrt()
            );
        }
    }

    println!("\n✅ Plugin integration example complete!");
    println!("\nTo use in a real plugin:");
    println!("1. Add this library to your JUCE/VST project");
    println!("2. Call prepare() in prepareToPlay()");
    println!("3. Call process_block() in processBlock()");
    println!("4. Set parameters from your UI controls");
}
