//! Owned bytes from a supervised development capture, not live acquisition authority.
//!
//! Validation establishes saved receipt/PNG correspondence only. The caller owns
//! collection, before/after process and directory checks, cancellation and restoration.
//! This module cannot construct observations, capture facts, admissions or effects.
use crate::Uuid;
use serde::Deserialize;
use sha2::{Digest, Sha256};
use std::{io::Cursor, sync::Arc};

pub const DISCOVERY_SCOPE: &str = "receiver-subtree-capture-unqualified-v11";
pub const QUALIFICATION: &str = "DevelopmentUnqualifiedHistoricalCorrespondence";
pub const SCHEMA: &str = "development-capture-observation/v2";
pub const MAX_COMPLETION_BYTES: usize = 8192;
pub const MAX_PNG_BYTES: usize = 8 * 1024 * 1024;
pub const MAX_PIXELS: u64 = 4_194_304;

/// Caller-selected correspondence expectations, never an observed page order or live guard.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ExpectedCaptureBinding {
    pub nonce: String,
    pub attempt_pid: String,
    pub attempt_start: String,
    pub root_device: String,
    pub root_inode: String,
    pub document_id: String,
    pub expected_order: Vec<String>,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DevelopmentCaptureError {
    Bound,
    Shape,
    Binding,
    Timing,
    Authority,
    Image,
}
impl std::fmt::Display for DevelopmentCaptureError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "development capture correspondence refused: {self:?}")
    }
}
impl std::error::Error for DevelopmentCaptureError {}

// Required fields, exact JSON types, duplicate fields and unknown fields fail closed.
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
struct Completion {
    kind: String,
    version: u32,
    nonce: String,
    attempt_pid: String,
    attempt_start: String,
    root_device: String,
    root_inode: String,
    setup_profile: String,
    setup_budget_ms: u64,
    capture_budget_ms: u64,
    accepted_ms: u64,
    baseline_ms: u64,
    grab_start_ms: u64,
    grab_end_ms: u64,
    post_read_ms: u64,
    completed_ms: u64,
    document_id: String,
    page_id: String,
    page_index: u32,
    begin_epoch: String,
    end_epoch: String,
    width: u32,
    height: u32,
    dpr: f64,
    image_width: u32,
    image_height: u32,
    png_bytes: u64,
    png_sha256: String,
    image_status: String,
    gui_callback_completed: bool,
    scope_current: bool,
    atomic_snapshot: bool,
    native_authority: bool,
    render_authority: bool,
    ui_acknowledged: bool,
    observed_order: bool,
    discovery_scope: String,
}

/// Immutable historical development evidence. Construction consumes the supplied
/// buffers once; it does not consume a device request or prevent receipt replay.
/// No `Platform`, `PageObservation`, `CapturedBatch` or admission conversion exists.
pub struct ReadOnlyDevelopmentCapture {
    completion: Completion,
    raw_completion: Arc<[u8]>,
    png: Arc<[u8]>,
    digest: [u8; 32],
    expected: ExpectedCaptureBinding,
}
impl std::fmt::Debug for ReadOnlyDevelopmentCapture {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("ReadOnlyDevelopmentCapture")
            .field("qualification", &QUALIFICATION)
            .field("dimensions", &self.dimensions())
            .finish_non_exhaustive()
    }
}

fn positive_decimal(value: &str) -> bool {
    !value.is_empty()
        && value.len() <= 20
        && matches!(value.as_bytes()[0], b'1'..=b'9')
        && value.bytes().all(|c| c.is_ascii_digit())
        && value.parse::<u64>().is_ok()
}
fn hex(value: &str, len: usize) -> bool {
    value.len() == len
        && value
            .bytes()
            .all(|c| c.is_ascii_digit() || (b'a'..=b'f').contains(&c))
}

impl ReadOnlyDevelopmentCapture {
    /// Validate exact v11 saved bytes against separately supplied collection bindings.
    /// Receipt assertions (including scope_current) describe the past attempt only.
    pub fn from_collected_v11(
        expected: &ExpectedCaptureBinding,
        completion: Vec<u8>,
        png: Vec<u8>,
    ) -> Result<Self, DevelopmentCaptureError> {
        use DevelopmentCaptureError as E;
        if completion.is_empty()
            || completion.len() > MAX_COMPLETION_BYTES
            || png.len() < 45
            || png.len() > MAX_PNG_BYTES
            || expected.expected_order.is_empty()
            || expected.expected_order.len() > 256
        {
            return Err(E::Bound);
        }
        if !hex(&expected.nonce, 32)
            || Uuid::parse(&expected.document_id).is_none()
            || expected
                .expected_order
                .iter()
                .any(|id| Uuid::parse(id).is_none())
            || [
                &expected.attempt_pid,
                &expected.attempt_start,
                &expected.root_device,
                &expected.root_inode,
            ]
            .iter()
            .any(|value| !positive_decimal(value))
            || expected
                .attempt_pid
                .parse::<u64>()
                .ok()
                .is_none_or(|pid| pid <= 1)
        {
            return Err(E::Binding);
        }
        let c: Completion = serde_json::from_slice(&completion).map_err(|_| E::Shape)?;
        if c.kind != "development-capture-observation"
            || c.version != 2
            || c.discovery_scope != DISCOVERY_SCOPE
            || c.setup_profile != "main-dev-facts-120s"
            || c.image_status != "available"
        {
            return Err(E::Shape);
        }
        if c.nonce != expected.nonce
            || c.attempt_pid != expected.attempt_pid
            || c.attempt_start != expected.attempt_start
            || c.root_device != expected.root_device
            || c.root_inode != expected.root_inode
            || c.document_id != expected.document_id
            || expected.expected_order.get(c.page_index as usize) != Some(&c.page_id)
            || !positive_decimal(&c.begin_epoch)
            || c.begin_epoch != c.end_epoch
        {
            return Err(E::Binding);
        }
        let times = [
            c.accepted_ms,
            c.baseline_ms,
            c.grab_start_ms,
            c.grab_end_ms,
            c.post_read_ms,
            c.completed_ms,
        ];
        if c.setup_budget_ms != 120_000
            || c.capture_budget_ms != 5_000
            || c.accepted_ms >= 120_000
            || c.completed_ms >= 120_000
            || c.completed_ms >= c.accepted_ms + 5_000
            || times.windows(2).any(|t| t[0] > t[1])
        {
            return Err(E::Timing);
        }
        if !c.gui_callback_completed
            || !c.scope_current
            || c.atomic_snapshot
            || c.native_authority
            || c.render_authority
            || c.ui_acknowledged
            || c.observed_order
        {
            return Err(E::Authority);
        }
        let pixels = u64::from(c.image_width) * u64::from(c.image_height);
        if c.width == 0
            || c.height == 0
            || c.image_width == 0
            || c.image_height == 0
            || [c.width, c.height, c.image_width, c.image_height]
                .iter()
                .any(|n| *n > i32::MAX as u32)
            || !c.dpr.is_finite()
            || c.dpr <= 0.0
            || pixels > MAX_PIXELS
            || f64::from(c.width) * f64::from(c.height) * c.dpr * c.dpr > MAX_PIXELS as f64
            || f64::from(c.image_width) != (f64::from(c.width) * c.dpr + 0.5).floor()
            || f64::from(c.image_height) != (f64::from(c.height) * c.dpr + 0.5).floor()
            || c.png_bytes != png.len() as u64
            || !hex(&c.png_sha256, 64)
        {
            return Err(E::Image);
        }
        let digest: [u8; 32] = Sha256::digest(&png).into();
        let expected_digest: String = digest.iter().map(|b| format!("{b:02x}")).collect();
        if c.png_sha256 != expected_digest
            || !png.starts_with(b"\x89PNG\r\n\x1a\n\0\0\0\rIHDR")
            || !png.ends_with(b"\0\0\0\0IEND\xaeB`\x82")
        {
            return Err(E::Image);
        }
        let mut reader =
            image::ImageReader::with_format(Cursor::new(&png), image::ImageFormat::Png);
        let mut limits = image::Limits::default();
        limits.max_image_width = Some(c.image_width);
        limits.max_image_height = Some(c.image_height);
        limits.max_alloc = Some(32 * 1024 * 1024);
        reader.limits(limits);
        let decoded = reader.decode().map_err(|_| E::Image)?;
        if [decoded.width(), decoded.height()] != [c.image_width, c.image_height] {
            return Err(E::Image);
        }
        Ok(Self {
            completion: c,
            raw_completion: completion.into(),
            png: png.into(),
            digest,
            expected: expected.clone(),
        })
    }

    pub fn png(&self) -> &[u8] {
        &self.png
    }
    pub fn raw_completion(&self) -> &[u8] {
        &self.raw_completion
    }
    pub fn png_bytes(&self) -> &[u8] {
        self.png()
    }
    pub fn completion_bytes(&self) -> &[u8] {
        self.raw_completion()
    }
    pub fn sha256(&self) -> &[u8; 32] {
        &self.digest
    }
    pub fn dimensions(&self) -> [u32; 2] {
        [self.completion.image_width, self.completion.image_height]
    }
    pub fn document_id(&self) -> &str {
        &self.completion.document_id
    }
    pub fn page_id(&self) -> &str {
        &self.completion.page_id
    }
    pub fn page_index(&self) -> u32 {
        self.completion.page_index
    }
    pub fn expected_binding(&self) -> &ExpectedCaptureBinding {
        &self.expected
    }
    pub fn discovery_scope(&self) -> &'static str {
        DISCOVERY_SCOPE
    }
    pub fn qualification(&self) -> &'static str {
        QUALIFICATION
    }
    pub fn schema(&self) -> &'static str {
        SCHEMA
    }
}
