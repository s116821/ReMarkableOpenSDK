//! Experimental identity/creation contract. No native operations are implemented.
//! The optional mock is synthetic and cannot establish device qualification.
#![forbid(unsafe_code)]

#[cfg(any(test, feature = "mock"))]
pub mod mock;

/// Persistent identity; operational device/instance scope is carried separately.
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct PageKey {
    pub document: Uuid,
    pub page: Uuid,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Uuid([u8; 16]);

impl Uuid {
    /// Accept canonical lowercase, nonnil UUID text; indices/titles are not IDs.
    pub fn parse(text: &str) -> Option<Self> {
        if text.len() != 36 {
            return None;
        }
        let mut bytes = [0; 16];
        let mut digits = 0;
        for (index, c) in text.bytes().enumerate() {
            if [8, 13, 18, 23].contains(&index) {
                if c != b'-' {
                    return None;
                }
            } else {
                let value = match c {
                    b'0'..=b'9' => c - b'0',
                    b'a'..=b'f' => c - b'a' + 10,
                    _ => return None,
                };
                bytes[digits / 2] = (bytes[digits / 2] << 4) | value;
                digits += 1;
            }
        }
        (bytes != [0; 16]).then_some(Self(bytes))
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct OperationId(pub Uuid);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum EvidenceOrigin {
    Synthetic,
}

/// Not constructible by external callers. Runtime and instance tokens are opaque.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct PageObservation {
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

impl PageObservation {
    pub fn page(&self) -> PageKey {
        self.page
    }
    pub fn order(&self) -> &[Uuid] {
        &self.order
    }
    pub fn origin(&self) -> EvidenceOrigin {
        self.origin
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TargetAllocation {
    ClientSelected(Uuid),
    NativeAssigned,
}

/// An immutable semantic request. A canonical wire fingerprint is not defined yet.
/// The mock compares the complete request, never a lossy hash or page position.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CreationRequest {
    operation: OperationId,
    source: PageObservation,
    allocation: TargetAllocation,
}

impl CreationRequest {
    pub fn new(
        operation: OperationId,
        source: PageObservation,
        allocation: TargetAllocation,
    ) -> Self {
        Self {
            operation,
            source,
            allocation,
        }
    }
    pub fn operation(&self) -> OperationId {
        self.operation
    }
    pub fn source(&self) -> &PageObservation {
        &self.source
    }
    pub fn allocation(&self) -> TargetAllocation {
        self.allocation
    }
}

/// Historical evidence only; never grants current rendering authority.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct CreationReceipt {
    request: CreationRequest,
    target: PageKey,
    after_order: Vec<Uuid>,
    revision: u64,
    origin: EvidenceOrigin,
}

impl CreationReceipt {
    pub fn request(&self) -> &CreationRequest {
        &self.request
    }
    pub fn target(&self) -> PageKey {
        self.target
    }
    pub fn after_order(&self) -> &[Uuid] {
        &self.after_order
    }
    pub fn origin(&self) -> EvidenceOrigin {
        self.origin
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RejectionStage {
    BeforeDispatch,
    ExecutorBeforeMutation,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum RejectionReason {
    ForeignScope,
    StaleGuard,
    AmbiguousIdentity,
    OperationConflict,
    TargetExists,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnsupportedReason {
    NoNativeAdapter,
    UnqualifiedMechanism,
    CapabilityNotImplemented,
}

/// Identity could not be established; never substitute a generated PageKey.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnknownIdentityReason {
    NoDocumentOpen,
    Unsettled,
    AmbiguousIdentity,
    MissingIdentifiers,
    InsufficientOwnershipEvidence,
}

/// Unsupported observation differs from an observation with unknown identity.
/// A known native page without a consumer conversation binding is neither error.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum ObservationFailure {
    Unsupported(UnsupportedReason),
    UnknownIdentity(UnknownIdentityReason),
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum CreationOutcome {
    Unsupported(UnsupportedReason),
    RejectedWithoutMutation {
        reason: RejectionReason,
        stage: RejectionStage,
    },
    CanceledBeforeDispatch,
    Committed(CreationReceipt),
    Indeterminate {
        operation: OperationId,
    },
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Reconciliation {
    Unsupported(UnsupportedReason),
    Rejected(RejectionReason),
    Unknown,
    Historical {
        receipt: Box<CreationReceipt>,
        current: Option<PageObservation>,
    },
}

/// First experimental slice. Capture/navigation remain unimplemented capabilities;
/// they must gain their full provenance/guard contract before becoming callable.
pub trait Platform {
    fn observe_page(&self) -> Result<PageObservation, ObservationFailure>;
    fn create_after(&mut self, request: &CreationRequest, canceled: bool) -> CreationOutcome;
    fn reconcile_creation(&self, request: &CreationRequest) -> Reconciliation;
}

/// Safe default for every native device until an adapter is qualified.
#[derive(Default)]
pub struct UnqualifiedPlatform;

impl Platform for UnqualifiedPlatform {
    fn observe_page(&self) -> Result<PageObservation, ObservationFailure> {
        Err(ObservationFailure::Unsupported(
            UnsupportedReason::NoNativeAdapter,
        ))
    }
    fn create_after(&mut self, _: &CreationRequest, _: bool) -> CreationOutcome {
        CreationOutcome::Unsupported(UnsupportedReason::NoNativeAdapter)
    }
    fn reconcile_creation(&self, _: &CreationRequest) -> Reconciliation {
        Reconciliation::Unsupported(UnsupportedReason::NoNativeAdapter)
    }
}
