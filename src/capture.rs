//! Immutable capture evidence model. There is no qualified native acquisition here.
//! Synthetic constructors are feature-gated and cannot produce native evidence.
use crate::{
    EvidenceOrigin, OperationId, PageObservation, UnknownIdentityReason, UnsupportedReason,
};
use std::{sync::Arc, time::Duration};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CaptureFailure {
    InvalidLimits,
    EncodedBound,
    DecodedBound,
    InvalidEncoding,
    InvalidGeometry,
    InvalidParent,
    PixelMismatch,
    OwnerChanged,
    InputChanged,
    StaleRender,
    DeadlineExpired,
    Canceled,
}
impl std::fmt::Display for CaptureFailure {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "capture evidence refused: {self:?}")
    }
}
impl std::error::Error for CaptureFailure {}

#[derive(Clone, Debug)]
pub enum CaptureOutcome {
    Unsupported(UnsupportedReason),
    /// Unknown identity cannot carry an unsupported-capability reason.
    /// ```compile_fail
    /// use remarkable_open_sdk::{capture::CaptureOutcome, UnsupportedReason};
    /// let contradictory = CaptureOutcome::UnknownIdentity(
    ///     UnsupportedReason::CapabilityNotImplemented);
    /// ```
    UnknownIdentity(UnknownIdentityReason),
    /// Only the optional test model can produce this outcome, never native success.
    Synthetic(Box<CapturedBatch>),
    Failed(CaptureFailure),
}

#[derive(Clone, Debug)]
pub struct CaptureRequest {
    operation: OperationId,
    source: PageObservation,
    deadline: Duration,
    canceled: bool,
}
impl CaptureRequest {
    pub fn new(
        operation: OperationId,
        source: PageObservation,
        deadline: Duration,
        canceled: bool,
    ) -> Self {
        Self {
            operation,
            source,
            deadline,
            canceled,
        }
    }
    pub fn operation(&self) -> OperationId {
        self.operation
    }
    pub fn source(&self) -> &PageObservation {
        &self.source
    }
    pub fn deadline(&self) -> Duration {
        self.deadline
    }
    pub fn canceled(&self) -> bool {
        self.canceled
    }
}

#[derive(Clone, Copy, Debug)]
pub struct CaptureLimits {
    pub max_encoded_image_bytes: usize,
    pub max_encoded_batch_bytes: usize,
    pub max_decoded_image_bytes: u64,
    pub max_width: u32,
    pub max_height: u32,
    /// Includes the native parent, overview and all details.
    pub max_images: usize,
}
impl Default for CaptureLimits {
    fn default() -> Self {
        Self {
            max_encoded_image_bytes: 32 * 1024 * 1024,
            max_encoded_batch_bytes: 64 * 1024 * 1024,
            max_decoded_image_bytes: 32 * 1024 * 1024,
            max_width: 8192,
            max_height: 8192,
            max_images: 8,
        }
    }
}
impl CaptureLimits {
    #[cfg(any(test, feature = "mock"))]
    fn validate(&self) -> Result<(), CaptureFailure> {
        if self.max_encoded_image_bytes == 0
            || self.max_encoded_image_bytes > 64 * 1024 * 1024
            || self.max_encoded_batch_bytes < self.max_encoded_image_bytes
            || self.max_encoded_batch_bytes > 128 * 1024 * 1024
            || self.max_decoded_image_bytes < 4
            || self.max_decoded_image_bytes > 128 * 1024 * 1024
            || self.max_width == 0
            || self.max_width > 32768
            || self.max_height == 0
            || self.max_height > 32768
            || self.max_images < 2
            || self.max_images > 16
        {
            return Err(CaptureFailure::InvalidLimits);
        }
        Ok(())
    }
    #[cfg(any(test, feature = "mock"))]
    fn dimensions(&self, width: u32, height: u32) -> Result<(), CaptureFailure> {
        if width == 0
            || height == 0
            || width > self.max_width
            || height > self.max_height
            || u64::from(width)
                .checked_mul(u64::from(height))
                .and_then(|n| n.checked_mul(4))
                .is_none_or(|n| n > self.max_decoded_image_bytes)
        {
            return Err(CaptureFailure::DecodedBound);
        }
        Ok(())
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct PixelRect {
    pub x: u32,
    pub y: u32,
    pub width: u32,
    pub height: u32,
}
impl PixelRect {
    fn validate(self, dimensions: [u32; 2]) -> Result<(), CaptureFailure> {
        if self.width == 0
            || self.height == 0
            || self
                .x
                .checked_add(self.width)
                .is_none_or(|x| x > dimensions[0])
            || self
                .y
                .checked_add(self.height)
                .is_none_or(|y| y > dimensions[1])
        {
            return Err(CaptureFailure::InvalidGeometry);
        }
        Ok(())
    }
}

/// Continuous pixel-edge coordinates; valid page content excludes chrome/padding.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct SourceRegion {
    pub x: f64,
    pub y: f64,
    pub width: f64,
    pub height: f64,
}
impl SourceRegion {
    fn validate(self, dimensions: [u32; 2]) -> Result<(), CaptureFailure> {
        if ![self.x, self.y, self.width, self.height]
            .iter()
            .all(|v| v.is_finite())
            || self.x < 0.0
            || self.y < 0.0
            || self.width <= 0.0
            || self.height <= 0.0
            || self.x + self.width > f64::from(dimensions[0])
            || self.y + self.height > f64::from(dimensions[1])
        {
            return Err(CaptureFailure::InvalidGeometry);
        }
        Ok(())
    }
    #[cfg(any(test, feature = "mock"))]
    fn contains(self, point: [f64; 2]) -> bool {
        point[0] >= self.x
            && point[1] >= self.y
            && point[0] <= self.x + self.width
            && point[1] <= self.y + self.height
    }
}

/// x'=a*x+c*y+e, y'=b*x+d*y+f. Invertibility is required; no clamping.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Affine([f64; 6]);
impl Affine {
    pub fn new(values: [f64; 6]) -> Result<Self, CaptureFailure> {
        let [a, b, c, d, _, _] = values;
        let determinant = a * d - b * c;
        if !values.iter().all(|v| v.is_finite()) || !determinant.is_finite() || determinant == 0.0 {
            return Err(CaptureFailure::InvalidGeometry);
        }
        Ok(Self(values))
    }
    pub fn coefficients(self) -> [f64; 6] {
        self.0
    }
    pub fn map(self, point: [f64; 2]) -> Result<[f64; 2], CaptureFailure> {
        let [a, b, c, d, e, f] = self.0;
        let result = [
            a * point[0] + c * point[1] + e,
            b * point[0] + d * point[1] + f,
        ];
        if result.iter().all(|v| v.is_finite()) {
            Ok(result)
        } else {
            Err(CaptureFailure::InvalidGeometry)
        }
    }
    pub fn inverse(self, point: [f64; 2]) -> Result<[f64; 2], CaptureFailure> {
        let [a, b, c, d, e, f] = self.0;
        let determinant = a * d - b * c;
        let [x, y] = [point[0] - e, point[1] - f];
        let result = [
            (d * x - c * y) / determinant,
            (-b * x + a * y) / determinant,
        ];
        if result.iter().all(|v| v.is_finite()) {
            Ok(result)
        } else {
            Err(CaptureFailure::InvalidGeometry)
        }
    }
    fn derived(self, crop: PixelRect, output: [u32; 2]) -> Result<Self, CaptureFailure> {
        if output.contains(&0) {
            return Err(CaptureFailure::InvalidGeometry);
        }
        let [a, b, c, d, e, f] = self.0;
        let sx = f64::from(crop.width) / f64::from(output[0]);
        let sy = f64::from(crop.height) / f64::from(output[1]);
        Self::new([
            a * sx,
            b * sx,
            c * sy,
            d * sy,
            a * f64::from(crop.x) + c * f64::from(crop.y) + e,
            b * f64::from(crop.x) + d * f64::from(crop.y) + f,
        ])
    }
}

/// Mathematical derivative descriptors only; never acquisition evidence or authority.
/// The current versioned crop/resize procedure preserves exact arithmetic and f64 bits.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct DerivedGeometry {
    affine: Affine,
    valid_source_region: SourceRegion,
}
impl DerivedGeometry {
    pub fn affine(self) -> Affine {
        self.affine
    }
    pub fn valid_source_region(self) -> SourceRegion {
        self.valid_source_region
    }
}

/// Derive structural geometry for the pinned crop/resize descriptor convention.
/// Compare historical descriptors by exact IEEE-754 bits, including signed zero.
/// The 1e-10 source-plane allowance does not relax descriptor equality or clamp values.
/// This function neither reads pixels nor establishes identity or native qualification.
pub fn derive_geometry(
    parent_dimensions: [u32; 2],
    parent_affine: Affine,
    parent_valid_region: SourceRegion,
    crop: PixelRect,
    output_dimensions: [u32; 2],
) -> Result<DerivedGeometry, CaptureFailure> {
    if parent_dimensions
        .into_iter()
        .chain(output_dimensions)
        .any(|axis| axis == 0 || axis > 32768)
    {
        return Err(CaptureFailure::InvalidGeometry);
    }
    crop.validate(parent_dimensions)?;
    validate_source_plane(parent_affine, parent_valid_region, parent_dimensions)?;
    let affine = parent_affine.derived(crop, output_dimensions)?;
    let valid_source_region = region_for_crop(parent_valid_region, crop, output_dimensions)?;
    validate_source_plane(affine, valid_source_region, output_dimensions)?;
    Ok(DerivedGeometry {
        affine,
        valid_source_region,
    })
}

fn region_for_crop(
    region: SourceRegion,
    crop: PixelRect,
    output: [u32; 2],
) -> Result<SourceRegion, CaptureFailure> {
    let x = region.x.max(f64::from(crop.x));
    let y = region.y.max(f64::from(crop.y));
    let right = (region.x + region.width).min(f64::from(crop.x) + f64::from(crop.width));
    let bottom = (region.y + region.height).min(f64::from(crop.y) + f64::from(crop.height));
    if x >= right || y >= bottom {
        return Err(CaptureFailure::InvalidGeometry);
    }
    let sx = f64::from(output[0]) / f64::from(crop.width);
    let sy = f64::from(output[1]) / f64::from(crop.height);
    Ok(SourceRegion {
        x: (x - f64::from(crop.x)) * sx,
        y: (y - f64::from(crop.y)) * sy,
        width: (right - x) * sx,
        height: (bottom - y) * sy,
    })
}
fn validate_source_plane(
    transform: Affine,
    region: SourceRegion,
    dimensions: [u32; 2],
) -> Result<(), CaptureFailure> {
    region.validate(dimensions)?;
    for point in [
        [region.x, region.y],
        [region.x + region.width, region.y],
        [region.x, region.y + region.height],
        [region.x + region.width, region.y + region.height],
    ] {
        // Roundoff tolerance is explicit; coordinates are never changed/clamped.
        if !transform
            .map(point)?
            .iter()
            .all(|v| *v >= -1e-10 && *v <= 1.0 + 1e-10)
        {
            return Err(CaptureFailure::InvalidGeometry);
        }
    }
    Ok(())
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ResizeFilter {
    Nearest,
    Triangle,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ImageRole {
    NativeParent,
    Overview,
    Detail(u32),
}
#[derive(Clone, Debug)]
pub struct Derivation {
    pub parent_digest: [u8; 32],
    pub parent_dimensions: [u32; 2],
    pub crop: PixelRect,
    pub output_dimensions: [u32; 2],
    pub filter: ResizeFilter,
    pub procedure: &'static str,
}

#[derive(Clone)]
pub struct CapturedImage {
    bytes: Arc<[u8]>,
    digest: [u8; 32],
    dimensions: [u32; 2],
    role: ImageRole,
    transform: Affine,
    valid_source: SourceRegion,
    derivation: Option<Derivation>,
}
impl std::fmt::Debug for CapturedImage {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.debug_struct("CapturedImage")
            .field("role", &self.role)
            .field("dimensions", &self.dimensions)
            .field("digest", &self.digest)
            .finish_non_exhaustive()
    }
}
impl CapturedImage {
    pub fn bytes(&self) -> &[u8] {
        &self.bytes
    }
    pub fn sha256(&self) -> &[u8; 32] {
        &self.digest
    }
    pub fn mime(&self) -> &'static str {
        "image/png"
    }
    pub fn dimensions(&self) -> [u32; 2] {
        self.dimensions
    }
    pub fn role(&self) -> ImageRole {
        self.role
    }
    pub fn transform(&self) -> Affine {
        self.transform
    }
    pub fn valid_source_region(&self) -> SourceRegion {
        self.valid_source
    }
    pub fn derivation(&self) -> Option<&Derivation> {
        self.derivation.as_ref()
    }
}

#[derive(Clone, Debug)]
pub struct CapturedBatch {
    operation: OperationId,
    source: PageObservation,
    interval: [Duration; 2],
    native: CapturedImage,
    images: Vec<CapturedImage>,
    target: Option<[f64; 2]>,
    buffer_generation: u64,
    viewport_revision: String,
    conversion_procedure: String,
    procedure_revision: String,
}
impl CapturedBatch {
    pub fn origin(&self) -> EvidenceOrigin {
        EvidenceOrigin::Synthetic
    }
    pub fn operation(&self) -> OperationId {
        self.operation
    }
    pub fn source(&self) -> &PageObservation {
        &self.source
    }
    pub fn interval(&self) -> [Duration; 2] {
        self.interval
    }
    pub fn native_parent(&self) -> &CapturedImage {
        &self.native
    }
    pub fn images(&self) -> &[CapturedImage] {
        &self.images
    }
    pub fn target(&self) -> Option<[f64; 2]> {
        self.target
    }
    pub fn buffer_generation(&self) -> u64 {
        self.buffer_generation
    }
    pub fn viewport_revision(&self) -> &str {
        &self.viewport_revision
    }
    pub fn conversion_procedure(&self) -> &str {
        &self.conversion_procedure
    }
    pub fn procedure_revision(&self) -> &str {
        &self.procedure_revision
    }
    pub fn encoded_bytes(&self) -> usize {
        self.native.bytes.len()
            + self
                .images
                .iter()
                .map(|image| image.bytes.len())
                .sum::<usize>()
    }
}

/// This model validates supplied synthetic observations, not actual frame freshness.
#[cfg(any(test, feature = "mock"))]
pub mod synthetic {
    use super::*;
    use image::{DynamicImage, ImageFormat, ImageReader};
    use sha2::{Digest, Sha256};
    use std::{collections::BTreeSet, io::Cursor};

    pub struct Acquisition {
        pub before: PageObservation,
        pub after: PageObservation,
        pub rendered_owner: PageObservation,
        pub render_generation: u64,
        pub buffer_generation: u64,
        pub input_changed: bool,
        pub interval: [Duration; 2],
        pub viewport_revision: String,
        pub conversion_procedure: String,
        pub procedure_revision: String,
        pub native_png: Vec<u8>,
        pub native_transform: Affine,
        pub valid_source_region: SourceRegion,
        pub target: Option<[f64; 2]>,
    }
    pub struct ImageInput {
        pub role: ImageRole,
        pub crop: PixelRect,
        pub output_dimensions: [u32; 2],
        pub filter: ResizeFilter,
        pub png: Vec<u8>,
    }

    // Read the fixed IHDR before invoking a decoder that may allocate pixel storage.
    fn header(bytes: &[u8], limits: CaptureLimits) -> Result<[u32; 2], CaptureFailure> {
        if bytes.len() > limits.max_encoded_image_bytes {
            return Err(CaptureFailure::EncodedBound);
        }
        if bytes.len() < 33
            || &bytes[..8] != b"\x89PNG\r\n\x1a\n"
            || &bytes[8..12] != 13u32.to_be_bytes().as_slice()
            || &bytes[12..16] != b"IHDR"
            || bytes[24] != 8
        {
            return Err(CaptureFailure::InvalidEncoding);
        }
        let width = u32::from_be_bytes(
            bytes[16..20]
                .try_into()
                .map_err(|_| CaptureFailure::InvalidEncoding)?,
        );
        let height = u32::from_be_bytes(
            bytes[20..24]
                .try_into()
                .map_err(|_| CaptureFailure::InvalidEncoding)?,
        );
        limits.dimensions(width, height)?;
        Ok([width, height])
    }
    fn decode(bytes: &[u8], limits: CaptureLimits) -> Result<DynamicImage, CaptureFailure> {
        let dimensions = header(bytes, limits)?;
        let mut reader = ImageReader::with_format(Cursor::new(bytes), ImageFormat::Png);
        let mut decode_limits = image::Limits::default();
        decode_limits.max_image_width = Some(limits.max_width);
        decode_limits.max_image_height = Some(limits.max_height);
        decode_limits.max_alloc = Some(limits.max_decoded_image_bytes);
        reader.limits(decode_limits);
        let image = reader
            .decode()
            .map_err(|_| CaptureFailure::InvalidEncoding)?;
        if [image.width(), image.height()] != dimensions {
            return Err(CaptureFailure::InvalidEncoding);
        }
        Ok(image)
    }
    fn image(
        bytes: Vec<u8>,
        role: ImageRole,
        dimensions: [u32; 2],
        transform: Affine,
        valid_source: SourceRegion,
        derivation: Option<Derivation>,
    ) -> CapturedImage {
        let digest = Sha256::digest(&bytes).into();
        CapturedImage {
            bytes: Arc::from(bytes),
            digest,
            dimensions,
            role,
            transform,
            valid_source,
            derivation,
        }
    }
    fn derived_pixels(native: &DynamicImage, input: &ImageInput) -> DynamicImage {
        let crop = input.crop;
        let cropped = native.crop_imm(crop.x, crop.y, crop.width, crop.height);
        if input.output_dimensions == [crop.width, crop.height] {
            cropped
        } else {
            cropped.resize_exact(
                input.output_dimensions[0],
                input.output_dimensions[1],
                match input.filter {
                    ResizeFilter::Nearest => image::imageops::FilterType::Nearest,
                    ResizeFilter::Triangle => image::imageops::FilterType::Triangle,
                },
            )
        }
    }
    /// Verify exact supplied encodings against trusted decoded parent derivation.
    /// PNG bytes are retained unchanged, even when equivalent encodings differ.
    pub fn validate_batch(
        request: &CaptureRequest,
        acquisition: Acquisition,
        inputs: Vec<ImageInput>,
        limits: CaptureLimits,
    ) -> Result<CapturedBatch, CaptureFailure> {
        limits.validate()?;
        if request.canceled {
            return Err(CaptureFailure::Canceled);
        }
        if acquisition.interval[0] > acquisition.interval[1]
            || acquisition.interval[1] >= request.deadline
        {
            return Err(CaptureFailure::DeadlineExpired);
        }
        if acquisition.before != request.source || acquisition.after != request.source {
            return Err(CaptureFailure::OwnerChanged);
        }
        if acquisition.input_changed {
            return Err(CaptureFailure::InputChanged);
        }
        if acquisition.rendered_owner != request.source
            || acquisition.buffer_generation == 0
            || acquisition.buffer_generation != acquisition.render_generation
            || acquisition.viewport_revision.is_empty()
            || acquisition.conversion_procedure.is_empty()
            || acquisition.procedure_revision.is_empty()
        {
            return Err(CaptureFailure::StaleRender);
        }
        if inputs.is_empty()
            || inputs
                .len()
                .checked_add(1)
                .is_none_or(|n| n > limits.max_images)
        {
            return Err(CaptureFailure::EncodedBound);
        }
        let total = inputs
            .iter()
            .try_fold(acquisition.native_png.len(), |sum, input| {
                sum.checked_add(input.png.len())
                    .ok_or(CaptureFailure::EncodedBound)
            })?;
        if total > limits.max_encoded_batch_bytes {
            return Err(CaptureFailure::EncodedBound);
        }
        let native_dimensions = header(&acquisition.native_png, limits)?;
        validate_source_plane(
            acquisition.native_transform,
            acquisition.valid_source_region,
            native_dimensions,
        )?;
        if let Some(target) = acquisition.target
            && (!target
                .iter()
                .all(|v| v.is_finite() && (0.0..=1.0).contains(v))
                || !acquisition
                    .valid_source_region
                    .contains(acquisition.native_transform.inverse(target)?))
        {
            return Err(CaptureFailure::InvalidGeometry);
        }
        let mut overview = false;
        let mut ordinals = BTreeSet::new();
        for input in &inputs {
            input.crop.validate(native_dimensions)?;
            limits.dimensions(input.output_dimensions[0], input.output_dimensions[1])?;
            if header(&input.png, limits)? != input.output_dimensions {
                return Err(CaptureFailure::InvalidEncoding);
            }
            match input.role {
                ImageRole::Overview
                    if !overview
                        && input.crop
                            == PixelRect {
                                x: 0,
                                y: 0,
                                width: native_dimensions[0],
                                height: native_dimensions[1],
                            } =>
                {
                    overview = true
                }
                ImageRole::Detail(ordinal) if ordinals.insert(ordinal) => {}
                _ => return Err(CaptureFailure::InvalidParent),
            }
        }
        if !overview {
            return Err(CaptureFailure::InvalidParent);
        }
        let pixels = decode(&acquisition.native_png, limits)?;
        let native = image(
            acquisition.native_png,
            ImageRole::NativeParent,
            native_dimensions,
            acquisition.native_transform,
            acquisition.valid_source_region,
            None,
        );
        let mut images = Vec::new();
        for input in inputs {
            let expected = derived_pixels(&pixels, &input);
            if expected.to_rgba8() != decode(&input.png, limits)?.to_rgba8() {
                return Err(CaptureFailure::PixelMismatch);
            }
            let geometry = derive_geometry(
                native_dimensions,
                native.transform,
                native.valid_source,
                input.crop,
                input.output_dimensions,
            )?;
            let transform = geometry.affine();
            let valid_source = geometry.valid_source_region();
            let derivation = Derivation {
                parent_digest: native.digest,
                parent_dimensions: native_dimensions,
                crop: input.crop,
                output_dimensions: input.output_dimensions,
                filter: input.filter,
                procedure: "image-0.25.10/crop-resize-8bit-v1",
            };
            images.push(image(
                input.png,
                input.role,
                input.output_dimensions,
                transform,
                valid_source,
                Some(derivation),
            ));
        }
        Ok(CapturedBatch {
            operation: request.operation,
            source: request.source.clone(),
            interval: acquisition.interval,
            native,
            images,
            target: acquisition.target,
            buffer_generation: acquisition.buffer_generation,
            viewport_revision: acquisition.viewport_revision,
            conversion_procedure: acquisition.conversion_procedure,
            procedure_revision: acquisition.procedure_revision,
        })
    }
}
