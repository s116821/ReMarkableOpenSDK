//! Logical actions and explicit synthetic completion, never native write authority.
use crate::{OperationId, PageKey, PageObservation, RejectionReason, UnsupportedReason};
use std::collections::HashSet;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LogicalDirection {
    Next,
    Previous,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum NavigationRejection {
    ForeignScope,
    StaleGuard,
    AmbiguousIdentity,
    NotAdjacent,
    OperationConflict,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct NavigationRequest {
    pub(crate) operation: OperationId,
    pub(crate) source: PageObservation,
    pub(crate) direction: LogicalDirection,
    pub(crate) target: PageKey,
}
impl NavigationRequest {
    pub fn new(
        operation: OperationId,
        source: PageObservation,
        direction: LogicalDirection,
        target: PageKey,
    ) -> Result<Self, NavigationRejection> {
        if source.page.document != target.document {
            return Err(NavigationRejection::ForeignScope);
        }
        if source.order.is_empty()
            || source.order.iter().collect::<HashSet<_>>().len() != source.order.len()
        {
            return Err(NavigationRejection::AmbiguousIdentity);
        }
        let index = source
            .order
            .iter()
            .position(|id| *id == source.page.page)
            .ok_or(NavigationRejection::AmbiguousIdentity)?;
        let neighbor = match direction {
            LogicalDirection::Next => index.checked_add(1),
            LogicalDirection::Previous => index.checked_sub(1),
        };
        if neighbor.and_then(|i| source.order.get(i)).copied() != Some(target.page) {
            return Err(NavigationRejection::NotAdjacent);
        }
        Ok(Self {
            operation,
            source,
            direction,
            target,
        })
    }
    pub fn operation(&self) -> OperationId {
        self.operation
    }
    pub fn source(&self) -> &PageObservation {
        &self.source
    }
    pub fn direction(&self) -> LogicalDirection {
        self.direction
    }
    pub fn target(&self) -> PageKey {
        self.target
    }
}

/// SDK-created historical model evidence. No public constructor or live authority.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct NavigationReceipt {
    pub(crate) request: NavigationRequest,
    pub(crate) observed: PageObservation,
}
impl NavigationReceipt {
    pub fn request(&self) -> &NavigationRequest {
        &self.request
    }
    pub fn observed(&self) -> &PageObservation {
        &self.observed
    }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum NavigationUncertainty {
    AlreadyDispatched,
    CanceledAfterDispatch,
    DeadlineExpired,
    WrongDestination,
    GuardLostAfterDispatch,
    UnreadyPixels,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum NavigationOutcome {
    Unsupported(UnsupportedReason),
    RejectedWithoutDispatch(NavigationRejection),
    CanceledBeforeDispatch,
    SyntheticVerified(NavigationReceipt),
    SyntheticUnchanged(NavigationReceipt),
    Indeterminate(NavigationUncertainty),
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum CreationHandoffOutcome {
    Unsupported(UnsupportedReason),
    Rejected(RejectionReason),
    /// Creation is already committed. This never means no native effect.
    CanceledAfterCreation,
    SyntheticAcquired(PageObservation),
}
