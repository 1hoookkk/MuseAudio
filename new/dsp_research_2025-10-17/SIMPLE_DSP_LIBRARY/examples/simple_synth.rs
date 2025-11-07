//! Simple synthesizer example

use simple_dsp::{EmuFilter, Oscillator, OscillatorWaveform};

fn main() {
    let sample_rate = 44100.0;
    let duration_seconds = 2.0;
    let samples = (sample_rate * duration_seconds) as usize;

    println!(
        "🎵 Generating {} seconds of audio ({:.0} samples)...",
        duration_seconds, samples
    );

    // Create oscillator
    let mut osc = Oscillator::new(sample_rate);
    osc.set_frequency(440.0); // A4 note
    osc.set_waveform(OscillatorWaveform::Sawtooth);

    // Create filter
    let mut filter = EmuFilter::new(sample_rate);
    filter.set_frequency(2000.0); // 2kHz cutoff
    filter.set_resonance(0.3);
    filter.set_smoothing(0.02);

    // Generate audio buffer
    let mut audio = vec![0.0f32; samples];

    println!("🎹 Playing simple melody...");

    // Simple melody: A4, C#5, E5, A5
    let notes = [440.0, 554.37, 659.25, 880.0];
    let note_duration = samples / notes.len();

    for (i, &note_freq) in notes.iter().enumerate() {
        let start = i * note_duration;
        let end = ((i + 1) * note_duration).min(samples);

        println!("  Note {}: {:.2} Hz", i + 1, note_freq);

        // Set oscillator frequency
        osc.set_frequency(note_freq);

        // Add some filter sweep during each note
        for sample_idx in start..end {
            let progress = (sample_idx - start) as f32 / note_duration as f32;

            // Sweep filter from low to high
            let filter_freq = 500.0 + 1500.0 * progress;
            filter.set_frequency(filter_freq);

            // Generate sample
            let mut sample = 0.0;
            osc.process(std::slice::from_mut(&mut sample));

            // Apply filter
            filter.process(std::slice::from_mut(&mut sample));

            // Apply simple envelope (fade in/out)
            let envelope = if progress < 0.1 {
                progress / 0.1 // Fade in
            } else if progress > 0.9 {
                (1.0 - progress) / 0.1 // Fade out
            } else {
                1.0
            };

            audio[sample_idx] = sample * envelope * 0.5; // Reduce volume
        }
    }

    // Calculate statistics
    let rms = (audio.iter().map(|x| x * x).sum::<f32>() / audio.len() as f32).sqrt();
    let peak = audio.iter().fold(0.0f32, |a, &b| a.abs().max(b.abs()));

    println!("\n📊 Audio statistics:");
    println!("  RMS level: {:.6}", rms);
    println!("  Peak level: {:.6}", peak);
    println!("  Dynamic range: {:.2} dB", 20.0 * (peak / rms).log10());

    // Test different waveforms
    println!("\n🎛️  Testing different waveforms (440 Hz)...");

    let waveforms = [
        (OscillatorWaveform::Sine, "Sine"),
        (OscillatorWaveform::Sawtooth, "Sawtooth"),
        (OscillatorWaveform::Square, "Square"),
        (OscillatorWaveform::Triangle, "Triangle"),
    ];

    for (waveform, name) in &waveforms {
        osc.set_waveform(*waveform);
        osc.reset();

        let mut test_samples = vec![0.0f32; 100];
        osc.process(&mut test_samples);

        let test_rms =
            (test_samples.iter().map(|x| x * x).sum::<f32>() / test_samples.len() as f32).sqrt();
        println!("  {}: RMS = {:.4}", name, test_rms);
    }

    // Demonstrate chord
    println!("\n🎼 Generating a simple chord (C major)...");

    let chord_freqs = [261.63, 329.63, 392.00]; // C4, E4, G4
    let mut chord_oscillators: Vec<Oscillator> = chord_freqs
        .iter()
        .map(|&freq| {
            let mut osc = Oscillator::new(sample_rate);
            osc.set_frequency(freq);
            osc.set_waveform(OscillatorWaveform::Sine);
            osc
        })
        .collect();

    let mut chord_audio = vec![0.0f32; sample_rate as usize]; // 1 second

    for sample in chord_audio.iter_mut() {
        let mut sum = 0.0;
        for osc in &mut chord_oscillators {
            let mut osc_sample = 0.0;
            osc.process(std::slice::from_mut(&mut osc_sample));
            sum += osc_sample;
        }
        *sample = (sum / 3.0) * 0.3; // Average and reduce volume
    }

    let chord_rms =
        (chord_audio.iter().map(|x| x * x).sum::<f32>() / chord_audio.len() as f32).sqrt();
    println!("  Chord RMS: {:.4}", chord_rms);

    println!("\n✅ Simple synthesizer example completed!");
    println!("💡 Try running: cargo run --example basic_filter");
}
