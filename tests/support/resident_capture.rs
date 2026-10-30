use remarkable_open_sdk::native_adapter::{HistoricalCapture, ReaderFacts};
use sha2::{Digest, Sha256};
use std::io::Cursor;

// Incompressible owned pixels exercise replies larger than ordinary controls.
pub fn capture(facts: ReaderFacts, width: u32, height: u32) -> HistoricalCapture {
    let mut state = 0x1234_5678u32;
    let pixels = image::RgbaImage::from_fn(width, height, |_, _| {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        image::Rgba(state.to_be_bytes())
    });
    let mut output = Cursor::new(Vec::new());
    pixels
        .write_to(&mut output, image::ImageFormat::Png)
        .unwrap();
    let bytes = output.into_inner();
    let digest = Sha256::digest(&bytes).into();
    HistoricalCapture::new(facts.clone(), facts, [width, height], 1.0, bytes, digest).unwrap()
}
