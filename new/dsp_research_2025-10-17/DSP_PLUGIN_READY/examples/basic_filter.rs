//! Basic filter example

use dsp_plugin::Filter;

fn main() {
    println!("🎛️ DSP Plugin Ready - Basic Filter Example\n");

    // Create filter at 44.1kHz
    let mut filter = Filter::new(44100.0);

    // Set filter parameters
    filter.set_frequency(1000.0); // 1kHz cutoff
    filter.set_resonance(0.7); // Medium resonance
    filter.set_smoothing(0.01); // Smooth parameter changes

    println!("Filter settings:");
    println!("  Frequency: {:.1} Hz", filter.frequency());
    println!("  Resonance: {:.2}", filter.resonance());

    // Create test signal (white noise)
    let mut audio = vec![0.0f32; 512];
    for (i, sample) in audio.iter_mut().enumerate() {
        *sample = (i as f32 * 0.1).sin(); // Simple test tone
    }

    println!("\nProcessing {} samples...", audio.len());

    // Process audio through filter
    filter.process(&mut audio);

    // Calculate output level
    let rms = (audio.iter().map(|x| x * x).sum::<f32>() / audio.len() as f32).sqrt();
    println!("Output RMS: {:.4}", rms);

    println!("\n✅ Filter processing complete!");
}
