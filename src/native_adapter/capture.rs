//! Owned, validated historical capture evidence. This type cannot grant a
//! PageObservation, input epoch, native write lease or persistence guarantee.
use super::{ProtocolError, ReaderFacts, Result};
use sha2::{Digest, Sha256};
use std::io::Cursor;

pub const MAX_CAPTURE_PNG: usize = 1_048_576;
pub const MAX_CAPTURE_PIXELS: u64 = 2_628_288;
pub const CAPTURE_PROCEDURE: &str = "rm2-reader-identity-full-order-png-v1";
pub const VISUAL_PROCEDURE: &str = "rm2-window-visual-png-v1";

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HistoricalCapture {
    before: ReaderFacts,
    after: ReaderFacts,
    visual: HistoricalVisual,
}
impl HistoricalCapture {
    /// Validate historical source samples and a bounded fully decoded PNG.
    /// Equal samples do not prove atomicity or exclude intervening changes.
    pub fn new(
        before: ReaderFacts,
        after: ReaderFacts,
        dimensions: [u32; 2],
        device_pixel_ratio: f64,
        png: Vec<u8>,
        sha256: [u8; 32],
    ) -> Result<Self> {
        if before != after || before.order().len() > 17 {
            return Err(ProtocolError::InvalidOrder);
        }
        Ok(Self {
            before,
            after,
            visual: HistoricalVisual::new(dimensions, device_pixel_ratio, png, sha256)?,
        })
    }
    pub fn before(&self) -> &ReaderFacts {
        &self.before
    }
    pub fn after(&self) -> &ReaderFacts {
        &self.after
    }
    pub fn dimensions(&self) -> [u32; 2] {
        self.visual.dimensions()
    }
    pub fn device_pixel_ratio(&self) -> f64 {
        self.visual.device_pixel_ratio()
    }
    pub fn png(&self) -> &[u8] {
        self.visual.png()
    }
    pub fn sha256(&self) -> [u8; 32] {
        self.visual.sha256()
    }
    pub fn procedure(&self) -> &'static str {
        CAPTURE_PROCEDURE
    }
}

/// Historical window pixels only. No document/page identity, source capture,
/// content freshness, persistence or capture/write permission is implied.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HistoricalVisual {
    dimensions: [u32; 2],
    dpr_bits: u64,
    png: Vec<u8>,
    sha256: [u8; 32],
}
impl HistoricalVisual {
    /// Validate peer-supplied image bytes with a bounded full PNG decode.
    /// Construction verifies the image, not its asserted native provenance.
    pub fn new(
        dimensions: [u32; 2],
        device_pixel_ratio: f64,
        png: Vec<u8>,
        sha256: [u8; 32],
    ) -> Result<Self> {
        let [width, height] = dimensions;
        if width == 0
            || height == 0
            || width > 1872
            || height > 1872
            || u64::from(width) * u64::from(height) > MAX_CAPTURE_PIXELS
            || !device_pixel_ratio.is_finite()
            || device_pixel_ratio <= 0.0
        {
            return Err(ProtocolError::InvalidImage);
        }
        if png.len() > MAX_CAPTURE_PNG {
            return Err(ProtocolError::TooLarge);
        }
        if png.len() < 45
            || &png[..8] != b"\x89PNG\r\n\x1a\n"
            || png[8..12] != 13u32.to_be_bytes()
            || &png[12..16] != b"IHDR"
            || png[16..20] != width.to_be_bytes()
            || png[20..24] != height.to_be_bytes()
            || Sha256::digest(&png).as_slice() != sha256
        {
            return Err(ProtocolError::InvalidImage);
        }
        // Require exactly one complete static PNG container. Decoders may stop
        // at the image data and otherwise overlook missing IEND/trailing bytes.
        let mut at = 8usize;
        let mut ended = false;
        while at < png.len() {
            if png.len() - at < 12 {
                return Err(ProtocolError::InvalidImage);
            }
            let count = u32::from_be_bytes(png[at..at + 4].try_into().unwrap()) as usize;
            if count > png.len() - at - 12 {
                return Err(ProtocolError::InvalidImage);
            }
            let tag = &png[at + 4..at + 8];
            if matches!(tag, b"acTL" | b"fcTL" | b"fdAT") {
                return Err(ProtocolError::InvalidImage);
            }
            at += count + 12;
            if tag == b"IEND" {
                if count != 0 || at != png.len() || png[at - 4..at] != [0xae, 0x42, 0x60, 0x82] {
                    return Err(ProtocolError::InvalidImage);
                }
                ended = true;
            }
        }
        if !ended {
            return Err(ProtocolError::InvalidImage);
        }
        let mut reader =
            image::ImageReader::with_format(Cursor::new(&png), image::ImageFormat::Png);
        let mut limits = image::Limits::default();
        limits.max_image_width = Some(1872);
        limits.max_image_height = Some(1872);
        limits.max_alloc = Some(10_513_152);
        reader.limits(limits);
        let decoded = reader.decode().map_err(|_| ProtocolError::InvalidImage)?;
        if [decoded.width(), decoded.height()] != dimensions {
            return Err(ProtocolError::InvalidImage);
        }
        Ok(Self {
            dimensions,
            dpr_bits: device_pixel_ratio.to_bits(),
            png,
            sha256,
        })
    }
    pub fn dimensions(&self) -> [u32; 2] {
        self.dimensions
    }
    pub fn device_pixel_ratio(&self) -> f64 {
        f64::from_bits(self.dpr_bits)
    }
    pub fn png(&self) -> &[u8] {
        &self.png
    }
    pub fn sha256(&self) -> [u8; 32] {
        self.sha256
    }
    pub fn procedure(&self) -> &'static str {
        VISUAL_PROCEDURE
    }
}
