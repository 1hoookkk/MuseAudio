//! Basic filter example

use simple_dsp::EmuFilter;

fn main() {
    let sample_rate = 44100.0;
    let mut filter = EmuFilter::new(sample_rate);

    // Set filter parameters
    filter.set_frequency(1000.0); // 1kHz cutoff
    filter.set_resonance(0.7); // Some resonance
    filter.set_smoothing(0.01); // Smooth parameter changes

    // Generate some test audio (white noise)
    let mut audio = vec![0.0f32; 1024];
    for sample in &mut audio {
        *sample = (rand::random::<f32>() - 0.5) * 2.0; // -1 to 1
    }

    println!("Processing {} samples of white noise...", audio.len());

    // Process audio through filter
    filter.process(&mut audio);

    // Calculate some basic statistics
    let rms = (audio.iter().map(|x| x * x).sum::<f32>() / audio.len() as f32).sqrt();
    let max = audio.iter().fold(0.0f32, |a, &b| a.abs().max(b.abs()));

    println!("Filter output:");
    println!("  RMS level: {:.4}", rms);
    println!("  Peak level: {:.4}", max);
    println!("  First 10 samples: {:?}", &audio[..10]);

    // Test parameter automation
    println!("\nTesting frequency sweep...");
    let mut sweep_filter = EmuFilter::new(sample_rate);

    for i in 0..100 {
        let freq = 200.0 * (i as f32 / 100.0).powf(2.0) + 100.0; // 100Hz to 2kHz sweep
        sweep_filter.set_frequency(freq);

        let mut chunk = vec![0.5f32; 64];
        sweep_filter.process(&mut chunk);

        if i % 20 == 0 {
            println!("  Step {}: freq={:.1}Hz, output={:.4}", i, freq, chunk[0]);
        }
    }

    println!("\n✅ Basic filter example completed!");
}

// Simple random number generator for demo
mod rand {
    use std::cell::Cell;
    use std::rc::Rc;

    thread_local! {
        static RNG_STATE: Rc<Cell<u32>> = Rc::new(Cell::new(12345));
    }

    pub fn random<T>() -> T
    where
        T: From<u32>,
    {
        RNG_STATE.with(|state| {
            let mut s = state.get();
            s ^= s << 13;
            s ^= s >> 17;
            s ^= s << 5;
            state.set(s);
            T::from(s)
        })
    }

    impl From<u32> for f32 {
        fn from(x: u32) -> Self {
            (x as f32) / (u32::MAX as f32)
        }
    }
}
