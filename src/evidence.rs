//! One-way historical facts. No facts value can become a live observation or guard.
use crate::{
    CreationReceipt, EvidenceOrigin, PageKey, PageObservation, TargetAllocation, Uuid, capture::*,
};

pub const SCHEMA: &str = "remarkable-open-sdk/evidence-facts/1";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ExportError {
    InvalidLimits,
    OrderBound,
    StringBound,
    MetadataBound,
    ImageBound,
}
impl std::fmt::Display for ExportError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "historical evidence export refused: {self:?}")
    }
}
impl std::error::Error for ExportError {}

#[derive(Clone, Copy, Debug)]
pub struct ExportLimits {
    pub max_order: usize,
    pub max_string_bytes: usize,
    pub max_variable_bytes: usize,
    pub max_images: usize,
}
impl Default for ExportLimits {
    fn default() -> Self {
        Self {
            max_order: 4096,
            max_string_bytes: 4096,
            max_variable_bytes: 64 * 1024,
            max_images: 16,
        }
    }
}
struct Budget {
    limits: ExportLimits,
    bytes: usize,
}
impl Budget {
    fn new(limits: ExportLimits) -> Result<Self, ExportError> {
        if limits.max_order == 0
            || limits.max_order > 4096
            || limits.max_string_bytes == 0
            || limits.max_string_bytes > 4096
            || limits.max_variable_bytes == 0
            || limits.max_variable_bytes > 64 * 1024
            || limits.max_images == 0
            || limits.max_images > 16
        {
            return Err(ExportError::InvalidLimits);
        }
        Ok(Self { limits, bytes: 0 })
    }
    fn add(&mut self, bytes: usize) -> Result<(), ExportError> {
        self.bytes = self
            .bytes
            .checked_add(bytes)
            .ok_or(ExportError::MetadataBound)?;
        if self.bytes > self.limits.max_variable_bytes {
            return Err(ExportError::MetadataBound);
        }
        Ok(())
    }
    fn order(&mut self, order: &[Uuid]) -> Result<(), ExportError> {
        if order.len() > self.limits.max_order {
            return Err(ExportError::OrderBound);
        }
        self.add(
            order
                .len()
                .checked_mul(16)
                .ok_or(ExportError::MetadataBound)?,
        )
    }
    fn string(&mut self, value: &str) -> Result<(), ExportError> {
        if value.len() > self.limits.max_string_bytes {
            return Err(ExportError::StringBound);
        }
        self.add(value.len())
    }
}

/// Stable schema names, never Rust Debug output or numeric discriminants.
pub fn origin_name(origin: EvidenceOrigin) -> &'static str {
    match origin {
        EvidenceOrigin::Synthetic => "Synthetic",
    }
}
pub fn allocation_name(allocation: TargetAllocation) -> &'static str {
    match allocation {
        TargetAllocation::ClientSelected(_) => "ClientSelected",
        TargetAllocation::NativeAssigned => "NativeAssigned",
    }
}
pub fn role_name(role: ImageRole) -> &'static str {
    match role {
        ImageRole::NativeParent => "NativeParent",
        ImageRole::Overview => "Overview",
        ImageRole::Detail(_) => "Detail",
    }
}
pub fn filter_name(filter: ResizeFilter) -> &'static str {
    match filter {
        ResizeFilter::Nearest => "Nearest",
        ResizeFilter::Triangle => "Triangle",
    }
}

macro_rules! copied {
    ($($name:ident : $ty:ty),* $(,)?) => { $(pub fn $name(&self) -> $ty { self.$name })* };
}
macro_rules! borrowed {
    ($($name:ident : $ty:ty),* $(,)?) => { $(pub fn $name(&self) -> &$ty { &self.$name })* };
}

/// Historical values only. Even operation constructors reject them.
/// ```compile_fail
/// use remarkable_open_sdk::{CreationRequest, OperationId, TargetAllocation, evidence::ObservationFacts};
/// fn resurrect(operation: OperationId, saved: ObservationFacts, allocation: TargetAllocation) {
///     CreationRequest::new(operation, saved, allocation);
/// }
/// ```
/// ```compile_fail
/// use remarkable_open_sdk::{capture::CaptureRequest, OperationId, evidence::ObservationFacts};
/// fn capture(operation: OperationId, saved: ObservationFacts) {
///     CaptureRequest::new(operation, saved, std::time::Duration::ZERO, false);
/// }
/// ```
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ObservationFacts {
    device: Uuid,
    instance: u64,
    session: u64,
    visit: u64,
    revision: u64,
    input_epoch: u64,
    page: PageKey,
    order: Vec<Uuid>,
    origin: EvidenceOrigin,
}
impl ObservationFacts {
    pub fn schema(&self) -> &'static str {
        SCHEMA
    }
    copied!(device: Uuid, instance: u64, session: u64, visit: u64, revision: u64, input_epoch: u64, page: PageKey, origin: EvidenceOrigin);
    pub fn order(&self) -> &[Uuid] {
        &self.order
    }
    pub fn qualification(&self) -> &'static str {
        "UnqualifiedSyntheticModel"
    }
}
fn observation(source: &PageObservation) -> ObservationFacts {
    ObservationFacts {
        device: source.device,
        instance: source.instance,
        session: source.session,
        visit: source.visit,
        revision: source.revision,
        input_epoch: source.input_epoch,
        page: source.page,
        order: source.order.clone(),
        origin: source.origin,
    }
}
impl PageObservation {
    pub fn export_facts(&self, limits: ExportLimits) -> Result<ObservationFacts, ExportError> {
        Budget::new(limits)?.order(&self.order)?;
        Ok(observation(self))
    }
}

/// ```compile_fail
/// use remarkable_open_sdk::{Platform, evidence::ReceiptFacts};
/// fn replay(platform: &mut impl Platform, saved: ReceiptFacts) {
///     platform.create_after(&saved, false);
/// }
/// ```
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ReceiptFacts {
    operation: Uuid,
    source: ObservationFacts,
    allocation: TargetAllocation,
    target: PageKey,
    after_order: Vec<Uuid>,
    revision: u64,
    origin: EvidenceOrigin,
}
impl ReceiptFacts {
    pub fn schema(&self) -> &'static str {
        SCHEMA
    }
    copied!(operation: Uuid, allocation: TargetAllocation, target: PageKey, revision: u64, origin: EvidenceOrigin);
    borrowed!(source: ObservationFacts);
    pub fn after_order(&self) -> &[Uuid] {
        &self.after_order
    }
    pub fn qualification(&self) -> &'static str {
        "UnqualifiedSyntheticModel"
    }
}
impl CreationReceipt {
    pub fn export_facts(&self, limits: ExportLimits) -> Result<ReceiptFacts, ExportError> {
        let mut budget = Budget::new(limits)?;
        budget.order(&self.request.source.order)?;
        budget.order(&self.after_order)?;
        Ok(ReceiptFacts {
            operation: self.request.operation.0,
            source: observation(&self.request.source),
            allocation: self.request.allocation,
            target: self.target,
            after_order: self.after_order.clone(),
            revision: self.revision,
            origin: self.origin,
        })
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct DurationFacts {
    seconds: u64,
    nanoseconds: u32,
}
impl DurationFacts {
    copied!(seconds: u64, nanoseconds: u32);
}
impl From<std::time::Duration> for DurationFacts {
    fn from(value: std::time::Duration) -> Self {
        Self {
            seconds: value.as_secs(),
            nanoseconds: value.subsec_nanos(),
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct DerivationFacts {
    parent_digest: [u8; 32],
    parent_dimensions: [u32; 2],
    crop: [u32; 4],
    output_dimensions: [u32; 2],
    filter: ResizeFilter,
    procedure: String,
}
impl DerivationFacts {
    copied!(parent_digest: [u8; 32], parent_dimensions: [u32; 2], crop: [u32; 4], output_dimensions: [u32; 2], filter: ResizeFilter);
    pub fn procedure(&self) -> &str {
        &self.procedure
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ImageFacts {
    role: ImageRole,
    bytes: u64,
    sha256: [u8; 32],
    dimensions: [u32; 2],
    affine_bits: [u64; 6],
    valid_region_bits: [u64; 4],
    derivation: Option<DerivationFacts>,
}
impl ImageFacts {
    copied!(role: ImageRole, bytes: u64, sha256: [u8; 32], dimensions: [u32; 2], affine_bits: [u64; 6], valid_region_bits: [u64; 4]);
    pub fn mime(&self) -> &'static str {
        "image/png"
    }
    pub fn derivation(&self) -> Option<&DerivationFacts> {
        self.derivation.as_ref()
    }
}
fn image(source: &CapturedImage) -> ImageFacts {
    let region = source.valid_source_region();
    ImageFacts {
        role: source.role(),
        bytes: source.bytes().len() as u64,
        sha256: *source.sha256(),
        dimensions: source.dimensions(),
        affine_bits: source.transform().coefficients().map(f64::to_bits),
        valid_region_bits: [region.x, region.y, region.width, region.height].map(f64::to_bits),
        derivation: source.derivation().map(|d| DerivationFacts {
            parent_digest: d.parent_digest,
            parent_dimensions: d.parent_dimensions,
            crop: [d.crop.x, d.crop.y, d.crop.width, d.crop.height],
            output_dimensions: d.output_dimensions,
            filter: d.filter,
            procedure: d.procedure.to_owned(),
        }),
    }
}

/// ```compile_fail
/// use remarkable_open_sdk::{Platform, evidence::CaptureFacts};
/// fn recapture(platform: &mut impl Platform, saved: CaptureFacts) {
///     platform.capture(&saved);
/// }
/// ```
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CaptureFacts {
    operation: Uuid,
    source: ObservationFacts,
    interval: [DurationFacts; 2],
    render_generation: u64,
    buffer_generation: u64,
    viewport_revision: String,
    conversion_procedure: String,
    procedure_revision: String,
    target_bits: Option<[u64; 2]>,
    native_parent: ImageFacts,
    images: Vec<ImageFacts>,
}
impl CaptureFacts {
    pub fn origin(&self) -> EvidenceOrigin {
        self.source.origin()
    }
    pub fn schema(&self) -> &'static str {
        SCHEMA
    }
    pub fn qualification(&self) -> &'static str {
        "UnqualifiedSyntheticModel"
    }
    pub fn clock_kind(&self) -> &'static str {
        "SyntheticInjectedDuration"
    }
    pub fn render_binding(&self) -> &'static str {
        "SyntheticEqualityAssertion"
    }
    copied!(operation: Uuid, interval: [DurationFacts; 2], render_generation: u64, buffer_generation: u64, target_bits: Option<[u64; 2]>);
    borrowed!(source: ObservationFacts, native_parent: ImageFacts);
    pub fn images(&self) -> &[ImageFacts] {
        &self.images
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
}
impl CapturedBatch {
    pub fn export_facts(&self, limits: ExportLimits) -> Result<CaptureFacts, ExportError> {
        let mut budget = Budget::new(limits)?;
        if self
            .images()
            .len()
            .checked_add(1)
            .is_none_or(|n| n > limits.max_images)
        {
            return Err(ExportError::ImageBound);
        }
        budget.order(self.source().order())?;
        budget.string(self.viewport_revision())?;
        budget.string(self.conversion_procedure())?;
        budget.string(self.procedure_revision())?;
        for image in std::iter::once(self.native_parent()).chain(self.images()) {
            if let Some(derivation) = image.derivation() {
                budget.string(derivation.procedure)?;
            }
        }
        // Every variable bound was checked before any string/order/vector clone.
        Ok(CaptureFacts {
            operation: self.operation().0,
            source: observation(self.source()),
            interval: self.interval().map(DurationFacts::from),
            render_generation: self.buffer_generation(),
            buffer_generation: self.buffer_generation(),
            viewport_revision: self.viewport_revision().to_owned(),
            conversion_procedure: self.conversion_procedure().to_owned(),
            procedure_revision: self.procedure_revision().to_owned(),
            target_bits: self.target().map(|target| target.map(f64::to_bits)),
            native_parent: image(self.native_parent()),
            images: self.images().iter().map(image).collect(),
        })
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::{CreationRequest, OperationId};
    fn id(n: u32) -> Uuid {
        Uuid::parse(&format!("{n:08x}-0000-0000-0000-000000000001")).unwrap()
    }
    fn source() -> PageObservation {
        PageObservation {
            device: id(1),
            instance: u64::MAX,
            session: (1u64 << 53) + 1,
            visit: u64::MAX - 1,
            revision: u64::MAX - 2,
            input_epoch: u64::MAX - 3,
            page: PageKey {
                document: id(2),
                page: id(3),
            },
            order: vec![id(3)],
            origin: EvidenceOrigin::Synthetic,
        }
    }
    #[test]
    fn original_high_values_and_duration_components_are_exact() {
        let live = source();
        let facts = live.export_facts(ExportLimits::default()).unwrap();
        assert_eq!(facts.instance(), u64::MAX);
        assert_eq!(facts.session(), (1u64 << 53) + 1);
        assert_eq!(facts.visit(), u64::MAX - 1);
        assert_eq!(facts.revision(), u64::MAX - 2);
        assert_eq!(facts.input_epoch(), u64::MAX - 3);
        assert_eq!(facts.device(), live.device);
        assert_eq!(facts.page(), live.page);
        assert_eq!(facts.order(), live.order);
        assert_eq!(facts.schema(), SCHEMA);
        assert_eq!(facts.qualification(), "UnqualifiedSyntheticModel");
        // An example consumer binary container uses integer bytes, never f64.
        let persisted = facts.session().to_le_bytes();
        assert_eq!(u64::from_le_bytes(persisted), (1u64 << 53) + 1);
        let duration = DurationFacts::from(std::time::Duration::new(u64::MAX, 999_999_999));
        assert_eq!(duration.seconds(), u64::MAX);
        assert_eq!(duration.nanoseconds(), 999_999_999);
    }
    #[test]
    fn receipt_retains_original_request_and_counts_both_orders() {
        for allocation in [
            TargetAllocation::ClientSelected(id(4)),
            TargetAllocation::NativeAssigned,
        ] {
            let source = source();
            let receipt = CreationReceipt {
                request: CreationRequest::new(OperationId(id(5)), source.clone(), allocation),
                target: PageKey {
                    document: id(2),
                    page: id(4),
                },
                after_order: vec![id(3), id(4)],
                revision: u64::MAX,
                origin: EvidenceOrigin::Synthetic,
            };
            assert!(matches!(
                receipt.export_facts(ExportLimits {
                    max_variable_bytes: 47,
                    ..ExportLimits::default()
                }),
                Err(ExportError::MetadataBound)
            ));
            let facts = receipt
                .export_facts(ExportLimits {
                    max_variable_bytes: 48,
                    ..ExportLimits::default()
                })
                .unwrap();
            assert_eq!(facts.operation(), id(5));
            assert_eq!(facts.allocation(), allocation);
            assert_eq!(
                facts.source(),
                &source.export_facts(ExportLimits::default()).unwrap()
            );
            assert_eq!(facts.target(), receipt.target);
            assert_eq!(facts.after_order(), receipt.after_order);
            assert_eq!(facts.revision(), u64::MAX);
            assert_eq!(facts.origin(), EvidenceOrigin::Synthetic);
        }
    }
    #[test]
    fn observation_scope_and_tight_order_limits_are_not_erased() {
        let source = source();
        let original = source.export_facts(ExportLimits::default()).unwrap();
        let mut other = source.clone();
        other.instance -= 1;
        assert_ne!(
            original,
            other.export_facts(ExportLimits::default()).unwrap()
        );
        other.device = id(9);
        assert_ne!(
            original,
            other.export_facts(ExportLimits::default()).unwrap()
        );
        other.order = vec![id(3); 4097];
        assert!(matches!(
            other.export_facts(ExportLimits::default()),
            Err(ExportError::OrderBound)
        ));
        assert!(matches!(
            source.export_facts(ExportLimits {
                max_variable_bytes: 15,
                ..ExportLimits::default()
            }),
            Err(ExportError::MetadataBound)
        ));
        assert!(matches!(
            source.export_facts(ExportLimits {
                max_order: usize::MAX,
                ..ExportLimits::default()
            }),
            Err(ExportError::InvalidLimits)
        ));
    }
}
