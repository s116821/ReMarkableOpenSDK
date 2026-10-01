//! Synthetic state-machine model, not a native adapter or durable store.
use crate::*;
use std::collections::{HashMap, HashSet};
use std::sync::atomic::{AtomicU64, Ordering};

static NEXT_INSTANCE: AtomicU64 = AtomicU64::new(1);

#[derive(Clone, Debug)]
pub enum ExecutionEvent {
    None,
    ExternalInput,
    ReverseOrder,
    CommitThenLoseReply,
}

pub struct MockPlatform {
    state: PageObservation,
    receipts: HashMap<OperationId, CreationReceipt>,
    event: ExecutionEvent,
    next_native_id: Option<Uuid>,
    can_enforce_guard: bool,
}

impl MockPlatform {
    pub fn new(device: Uuid, document: Uuid, order: Vec<Uuid>, current: Uuid) -> Self {
        Self {
            state: PageObservation {
                device,
                instance: NEXT_INSTANCE.fetch_add(1, Ordering::Relaxed),
                session: 1,
                visit: 1,
                revision: 1,
                input_epoch: 0,
                page: PageKey {
                    document,
                    page: current,
                },
                order,
                origin: EvidenceOrigin::Synthetic,
            },
            receipts: HashMap::new(),
            event: ExecutionEvent::None,
            next_native_id: None,
            can_enforce_guard: true,
        }
    }
    pub fn inject_at_execution(&mut self, event: ExecutionEvent) {
        self.event = event;
    }
    pub fn assign_next_native_id(&mut self, id: Uuid) {
        self.next_native_id = Some(id);
    }
    pub fn set_guard_enforcement(&mut self, supported: bool) {
        self.can_enforce_guard = supported;
    }
    pub fn pages(&self) -> &[Uuid] {
        &self.state.order
    }
    pub fn remove_page(&mut self, page: Uuid) {
        self.state.order.retain(|candidate| *candidate != page);
        self.state.revision += 1;
    }
    fn scope_matches(&self, source: &PageObservation) -> bool {
        source.device == self.state.device && source.instance == self.state.instance
    }
    fn valid_identity(&self) -> bool {
        !self.state.order.is_empty()
            && self.state.order.iter().collect::<HashSet<_>>().len() == self.state.order.len()
            && self.state.order.contains(&self.state.page.page)
    }
    fn reject(reason: RejectionReason, stage: RejectionStage) -> CreationOutcome {
        CreationOutcome::RejectedWithoutMutation { reason, stage }
    }
}

impl Platform for MockPlatform {
    fn observe_page(&self) -> Result<PageObservation, UnsupportedReason> {
        if !self.valid_identity() {
            return Err(UnsupportedReason::UnqualifiedMechanism);
        }
        Ok(self.state.clone())
    }

    fn create_after(&mut self, request: &CreationRequest, canceled: bool) -> CreationOutcome {
        use RejectionReason::*;
        use RejectionStage::*;
        if !self.can_enforce_guard {
            return CreationOutcome::Unsupported(UnsupportedReason::UnqualifiedMechanism);
        }
        if !self.scope_matches(&request.source) {
            return Self::reject(ForeignScope, BeforeDispatch);
        }
        // A receipt is historical. Look up immutable request before checking its
        // now-stale source revision, so a retry never creates a duplicate.
        if let Some(receipt) = self.receipts.get(&request.operation) {
            return if receipt.request == *request {
                CreationOutcome::Committed(receipt.clone())
            } else {
                Self::reject(OperationConflict, BeforeDispatch)
            };
        }
        if canceled {
            return CreationOutcome::CanceledBeforeDispatch;
        }
        if !self.valid_identity() {
            return Self::reject(AmbiguousIdentity, BeforeDispatch);
        }
        if request.source != self.state {
            return Self::reject(StaleGuard, BeforeDispatch);
        }

        // Represents an executor queue boundary; deliberately permit a race here.
        let event = std::mem::replace(&mut self.event, ExecutionEvent::None);
        match event {
            ExecutionEvent::ExternalInput => self.state.input_epoch += 1,
            ExecutionEvent::ReverseOrder => {
                self.state.order.reverse();
                self.state.revision += 1;
            }
            _ => {}
        }
        // The model's atomic compare-and-act. A real adapter must qualify an
        // equivalent native serialization guarantee, not copy this host check.
        if request.source != self.state {
            return Self::reject(StaleGuard, ExecutorBeforeMutation);
        }
        let target = match request.allocation {
            TargetAllocation::ClientSelected(id) => id,
            TargetAllocation::NativeAssigned => match self.next_native_id {
                Some(id) => id,
                None => {
                    return CreationOutcome::Unsupported(UnsupportedReason::UnqualifiedMechanism);
                }
            },
        };
        if self.state.order.contains(&target) {
            return Self::reject(TargetExists, ExecutorBeforeMutation);
        }
        let position = self
            .state
            .order
            .iter()
            .position(|id| *id == request.source.page.page)
            .unwrap();
        self.state.order.insert(position + 1, target);
        self.state.revision += 1;
        let receipt = CreationReceipt {
            request: request.clone(),
            target: PageKey {
                document: self.state.page.document,
                page: target,
            },
            after_order: self.state.order.clone(),
            revision: self.state.revision,
            origin: EvidenceOrigin::Synthetic,
        };
        // In-memory model of atomic insertion + correlation, not durability proof.
        self.receipts.insert(request.operation, receipt.clone());
        if matches!(request.allocation, TargetAllocation::NativeAssigned) {
            self.next_native_id = None;
        }
        if matches!(event, ExecutionEvent::CommitThenLoseReply) {
            CreationOutcome::Indeterminate {
                operation: request.operation,
            }
        } else {
            CreationOutcome::Committed(receipt)
        }
    }

    fn reconcile_creation(&self, request: &CreationRequest) -> Reconciliation {
        if !self.scope_matches(&request.source) {
            return Reconciliation::Rejected(RejectionReason::ForeignScope);
        }
        match self.receipts.get(&request.operation) {
            Some(receipt) if receipt.request == *request => Reconciliation::Historical {
                receipt: Box::new(receipt.clone()),
                current: self.observe_page().ok(),
            },
            Some(_) => Reconciliation::Rejected(RejectionReason::OperationConflict),
            None => Reconciliation::Unknown,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn id(n: u8) -> Uuid {
        Uuid([n; 16])
    }
    fn platform() -> MockPlatform {
        MockPlatform::new(id(1), id(2), vec![id(3), id(4)], id(3))
    }
    fn request(p: &MockPlatform, allocation: TargetAllocation) -> CreationRequest {
        CreationRequest::new(OperationId(id(9)), p.observe_page().unwrap(), allocation)
    }

    #[test]
    fn dispatch_execution_race_never_inserts() {
        for event in [ExecutionEvent::ExternalInput, ExecutionEvent::ReverseOrder] {
            let mut p = platform();
            let req = request(&p, TargetAllocation::ClientSelected(id(5)));
            p.inject_at_execution(event);
            assert_eq!(
                p.create_after(&req, false),
                MockPlatform::reject(
                    RejectionReason::StaleGuard,
                    RejectionStage::ExecutorBeforeMutation
                )
            );
            assert!(!p.pages().contains(&id(5)));
            assert_eq!(p.pages().len(), 2);
        }
    }

    #[test]
    fn foreign_device_and_recreated_instance_reject_coincident_ids() {
        let a = platform();
        let req = request(&a, TargetAllocation::ClientSelected(id(5)));
        let mut b = MockPlatform::new(id(7), id(2), vec![id(3), id(4)], id(3));
        let mut recreated = platform();
        for p in [&mut b, &mut recreated] {
            assert_eq!(
                p.create_after(&req, false),
                MockPlatform::reject(
                    RejectionReason::ForeignScope,
                    RejectionStage::BeforeDispatch
                )
            );
            assert_eq!(p.pages().len(), 2);
        }
    }

    #[test]
    fn native_assigned_lost_reply_reconciles_without_replay() {
        let mut p = platform();
        p.assign_next_native_id(id(5));
        let req = request(&p, TargetAllocation::NativeAssigned);
        p.inject_at_execution(ExecutionEvent::CommitThenLoseReply);
        assert!(matches!(
            p.create_after(&req, false),
            CreationOutcome::Indeterminate { .. }
        ));
        let Reconciliation::Historical { receipt, .. } = p.reconcile_creation(&req) else {
            panic!()
        };
        assert_eq!(receipt.target().page, id(5));
        assert_eq!(receipt.origin(), EvidenceOrigin::Synthetic);
        assert!(matches!(
            p.create_after(&req, false),
            CreationOutcome::Committed(_)
        ));
        assert_eq!(p.pages(), &[id(3), id(5), id(4)]);
    }

    #[test]
    fn removed_target_is_not_resurrected_by_historical_receipt() {
        let mut p = platform();
        let req = request(&p, TargetAllocation::ClientSelected(id(5)));
        assert!(matches!(
            p.create_after(&req, false),
            CreationOutcome::Committed(_)
        ));
        p.remove_page(id(5));
        assert!(matches!(
            p.create_after(&req, false),
            CreationOutcome::Committed(_)
        ));
        assert_eq!(p.pages(), &[id(3), id(4)]);
        let Reconciliation::Historical {
            receipt,
            current: Some(current),
        } = p.reconcile_creation(&req)
        else {
            panic!()
        };
        assert!(receipt.after_order().contains(&id(5)));
        assert!(!current.order().contains(&id(5)));
    }

    #[test]
    fn operation_identity_cannot_be_reused_for_a_changed_request() {
        let mut p = platform();
        let req = request(&p, TargetAllocation::ClientSelected(id(5)));
        p.create_after(&req, false);
        let changed = request(&p, TargetAllocation::ClientSelected(id(6)));
        assert_eq!(
            p.create_after(&changed, false),
            MockPlatform::reject(
                RejectionReason::OperationConflict,
                RejectionStage::BeforeDispatch
            )
        );
        assert!(!p.pages().contains(&id(6)));
    }

    #[test]
    fn unsupported_and_canceled_do_not_mutate() {
        let mut p = platform();
        let req = request(&p, TargetAllocation::ClientSelected(id(5)));
        assert_eq!(
            p.create_after(&req, true),
            CreationOutcome::CanceledBeforeDispatch
        );
        p.set_guard_enforcement(false);
        assert!(matches!(
            p.create_after(&req, false),
            CreationOutcome::Unsupported(_)
        ));
        assert_eq!(p.pages().len(), 2);
        assert!(matches!(
            UnqualifiedPlatform.create_after(&req, false),
            CreationOutcome::Unsupported(_)
        ));
    }

    #[test]
    fn uuid_rejects_noncanonical_and_path_values() {
        assert!(Uuid::parse("12345678-1234-1234-abcd-123456789abc").is_some());
        for bad in [
            "../page",
            "12345678-1234-1234-ABCD-123456789ABC",
            "12345678/1234-1234-abcd-123456789abc",
        ] {
            assert!(Uuid::parse(bad).is_none());
        }
    }
}
