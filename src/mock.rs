//! Synthetic state-machine model, not a native adapter or durable store.
use crate::navigation::*;
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

#[derive(Clone, Copy, Debug, Default)]
pub enum NavigationFault {
    #[default]
    None,
    BeforeDispatchInput,
    BeforeDispatchOrder,
    BeforeDispatchSession,
    BeforeDispatchVisit,
    NoMovement,
    WrongNeighbor,
    AfterDispatchInput,
    AfterDispatchSession,
    AfterDispatchOrder,
    AfterDispatchVisit,
    CanceledAfterDispatch,
    Deadline,
    UnreadyPixels,
}

pub struct MockPlatform {
    state: PageObservation,
    receipts: HashMap<OperationId, CreationReceipt>,
    event: ExecutionEvent,
    next_native_id: Option<Uuid>,
    can_enforce_guard: bool,
    navigation_operations: HashMap<OperationId, NavigationRequest>,
    handed_off: HashSet<OperationId>,
    navigation_fault: NavigationFault,
    gesture_count: usize,
    creation_selects_target: bool,
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
            navigation_operations: HashMap::new(),
            handed_off: HashSet::new(),
            navigation_fault: NavigationFault::None,
            gesture_count: 0,
            creation_selects_target: false,
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
    pub fn inject_navigation_fault(&mut self, fault: NavigationFault) {
        self.navigation_fault = fault;
    }
    pub fn gesture_count(&self) -> usize {
        self.gesture_count
    }
    pub fn set_creation_selects_target(&mut self, selected: bool) {
        self.creation_selects_target = selected;
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
    fn acquire_after_creation(
        &mut self,
        receipt: &CreationReceipt,
        canceled: bool,
    ) -> CreationHandoffOutcome {
        use CreationHandoffOutcome::*;
        let source = &receipt.request.source;
        if !self.can_enforce_guard {
            return Unsupported(UnsupportedReason::UnqualifiedMechanism);
        }
        if !self.scope_matches(source) {
            return Rejected(RejectionReason::ForeignScope);
        }
        if self.receipts.get(&receipt.request.operation) != Some(receipt) {
            return Rejected(RejectionReason::OperationConflict);
        }
        if self.handed_off.contains(&receipt.request.operation) {
            return Rejected(RejectionReason::StaleGuard);
        }
        // Consume the correlation before cancellation or stale-state refusal:
        // no failed handoff can later refresh authority for this operation.
        self.handed_off.insert(receipt.request.operation);
        if canceled {
            return CanceledAfterCreation;
        }
        let active_source = self.state.page == source.page && self.state.visit == source.visit;
        let active_target = self.state.page == receipt.target
            && source.visit.checked_add(1) == Some(self.state.visit);
        if !self.valid_identity()
            || self.state.session != source.session
            || self.state.input_epoch != source.input_epoch
            || self.state.revision != receipt.revision
            || self.state.order != receipt.after_order
            || (!active_source && !active_target)
        {
            return Rejected(RejectionReason::StaleGuard);
        }
        SyntheticAcquired(self.state.clone())
    }

    fn navigate(&mut self, request: &NavigationRequest, canceled: bool) -> NavigationOutcome {
        use NavigationOutcome::*;
        if !self.can_enforce_guard {
            return Unsupported(UnsupportedReason::UnqualifiedMechanism);
        }
        if !self.scope_matches(&request.source) {
            return RejectedWithoutDispatch(NavigationRejection::ForeignScope);
        }
        if let Some(previous) = self.navigation_operations.get(&request.operation) {
            return if previous == request {
                Indeterminate(NavigationUncertainty::AlreadyDispatched)
            } else {
                RejectedWithoutDispatch(NavigationRejection::OperationConflict)
            };
        }
        if canceled {
            return CanceledBeforeDispatch;
        }
        if request.source != self.state {
            return RejectedWithoutDispatch(NavigationRejection::StaleGuard);
        }
        let fault = std::mem::take(&mut self.navigation_fault);
        match fault {
            NavigationFault::BeforeDispatchInput => self.state.input_epoch += 1,
            NavigationFault::BeforeDispatchOrder => {
                self.state.order.reverse();
                self.state.revision += 1;
            }
            NavigationFault::BeforeDispatchSession => self.state.session += 1,
            NavigationFault::BeforeDispatchVisit => self.state.visit += 1,
            _ => {}
        }
        // Synthetic atomic execution guard, not a native compare-and-act proof.
        if request.source != self.state {
            return RejectedWithoutDispatch(NavigationRejection::StaleGuard);
        }
        if NavigationRequest::new(
            request.operation,
            self.state.clone(),
            request.direction,
            request.target,
        )
        .is_err()
        {
            return RejectedWithoutDispatch(NavigationRejection::NotAdjacent);
        }
        self.navigation_operations
            .insert(request.operation, request.clone());
        self.gesture_count += 1;
        if matches!(fault, NavigationFault::NoMovement) {
            return SyntheticUnchanged(NavigationReceipt {
                request: request.clone(),
                observed: self.state.clone(),
            });
        }
        self.state.page = request.target;
        self.state.visit += 1;
        let uncertainty = match fault {
            NavigationFault::WrongNeighbor => {
                self.state.page.page = request.source.page.page;
                Some(NavigationUncertainty::WrongDestination)
            }
            NavigationFault::AfterDispatchInput => {
                self.state.input_epoch += 1;
                Some(NavigationUncertainty::GuardLostAfterDispatch)
            }
            NavigationFault::AfterDispatchSession => {
                self.state.session += 1;
                Some(NavigationUncertainty::GuardLostAfterDispatch)
            }
            NavigationFault::AfterDispatchOrder => {
                self.state.order.reverse();
                self.state.revision += 1;
                Some(NavigationUncertainty::GuardLostAfterDispatch)
            }
            NavigationFault::AfterDispatchVisit => {
                self.state.visit += 1;
                Some(NavigationUncertainty::GuardLostAfterDispatch)
            }
            NavigationFault::CanceledAfterDispatch => {
                Some(NavigationUncertainty::CanceledAfterDispatch)
            }
            NavigationFault::Deadline => Some(NavigationUncertainty::DeadlineExpired),
            NavigationFault::UnreadyPixels => Some(NavigationUncertainty::UnreadyPixels),
            _ => None,
        };
        if let Some(reason) = uncertainty {
            return Indeterminate(reason);
        }
        SyntheticVerified(NavigationReceipt {
            request: request.clone(),
            observed: self.state.clone(),
        })
    }
    fn observe_page(&self) -> Result<PageObservation, ObservationFailure> {
        if self.state.order.is_empty() {
            return Err(ObservationFailure::UnknownIdentity(
                UnknownIdentityReason::NoDocumentOpen,
            ));
        }
        if self.state.order.iter().collect::<HashSet<_>>().len() != self.state.order.len() {
            return Err(ObservationFailure::UnknownIdentity(
                UnknownIdentityReason::AmbiguousIdentity,
            ));
        }
        if !self.state.order.contains(&self.state.page.page) {
            return Err(ObservationFailure::UnknownIdentity(
                UnknownIdentityReason::Unsettled,
            ));
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
        if self.creation_selects_target {
            self.state.page.page = target;
            self.state.visit += 1;
        }
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
mod navigation_tests {
    use super::*;
    fn id(n: u8) -> Uuid {
        Uuid([n; 16])
    }
    fn platform() -> MockPlatform {
        MockPlatform::new(id(1), id(2), vec![id(3), id(4), id(5)], id(3))
    }
    fn next(p: &MockPlatform) -> NavigationRequest {
        NavigationRequest::new(
            OperationId(id(10)),
            p.observe_page().unwrap(),
            LogicalDirection::Next,
            PageKey {
                document: id(2),
                page: id(4),
            },
        )
        .unwrap()
    }
    fn create(p: &mut MockPlatform) -> CreationReceipt {
        let request = CreationRequest::new(
            OperationId(id(9)),
            p.observe_page().unwrap(),
            TargetAllocation::ClientSelected(id(6)),
        );
        match p.create_after(&request, false) {
            CreationOutcome::Committed(r) => r,
            _ => panic!("model creation"),
        }
    }
    #[test]
    fn logical_neighbors_and_single_dispatch_correlation() {
        let mut p = platform();
        let request = next(&p);
        let NavigationOutcome::SyntheticVerified(receipt) = p.navigate(&request, false) else {
            panic!("next")
        };
        assert_eq!(receipt.observed().page(), request.target());
        assert_eq!(p.gesture_count(), 1);
        assert_ne!(receipt.observed.visit, request.source.visit);
        assert_eq!(
            p.navigate(&request, false),
            NavigationOutcome::Indeterminate(NavigationUncertainty::AlreadyDispatched)
        );
        let mut changed = request.clone();
        changed.direction = LogicalDirection::Previous;
        assert_eq!(
            p.navigate(&changed, false),
            NavigationOutcome::RejectedWithoutDispatch(NavigationRejection::OperationConflict)
        );
        assert_eq!(p.gesture_count(), 1);
        let previous = NavigationRequest::new(
            OperationId(id(11)),
            p.observe_page().unwrap(),
            LogicalDirection::Previous,
            request.source.page(),
        )
        .unwrap();
        assert!(matches!(
            p.navigate(&previous, false),
            NavigationOutcome::SyntheticVerified(_)
        ));
        assert_eq!(p.gesture_count(), 2);
    }
    #[test]
    fn constructors_and_execution_guards_reject_without_gesture() {
        let p = platform();
        let source = p.observe_page().unwrap();
        for (direction, target) in [
            (LogicalDirection::Previous, id(4)),
            (LogicalDirection::Next, id(5)),
        ] {
            assert_eq!(
                NavigationRequest::new(
                    OperationId(id(10)),
                    source.clone(),
                    direction,
                    PageKey {
                        document: id(2),
                        page: target
                    }
                ),
                Err(NavigationRejection::NotAdjacent)
            );
        }
        assert_eq!(
            NavigationRequest::new(
                OperationId(id(10)),
                source,
                LogicalDirection::Next,
                PageKey {
                    document: id(8),
                    page: id(4)
                }
            ),
            Err(NavigationRejection::ForeignScope)
        );
        for fault in [
            NavigationFault::BeforeDispatchInput,
            NavigationFault::BeforeDispatchOrder,
            NavigationFault::BeforeDispatchSession,
            NavigationFault::BeforeDispatchVisit,
        ] {
            let mut p = platform();
            let r = next(&p);
            p.inject_navigation_fault(fault);
            assert_eq!(
                p.navigate(&r, false),
                NavigationOutcome::RejectedWithoutDispatch(NavigationRejection::StaleGuard)
            );
            assert_eq!(p.gesture_count(), 0);
        }
        let mut foreign = platform();
        assert_eq!(
            foreign.navigate(&next(&p), false),
            NavigationOutcome::RejectedWithoutDispatch(NavigationRejection::ForeignScope)
        );
        let mut p = platform();
        let r = next(&p);
        assert_eq!(
            p.navigate(&r, true),
            NavigationOutcome::CanceledBeforeDispatch
        );
        assert_eq!(p.gesture_count(), 0);
    }
    #[test]
    fn postdispatch_faults_never_claim_no_effect_or_repeat() {
        for fault in [
            NavigationFault::WrongNeighbor,
            NavigationFault::AfterDispatchInput,
            NavigationFault::AfterDispatchSession,
            NavigationFault::AfterDispatchOrder,
            NavigationFault::AfterDispatchVisit,
            NavigationFault::CanceledAfterDispatch,
            NavigationFault::Deadline,
            NavigationFault::UnreadyPixels,
        ] {
            let mut p = platform();
            let r = next(&p);
            p.inject_navigation_fault(fault);
            assert!(matches!(
                p.navigate(&r, false),
                NavigationOutcome::Indeterminate(_)
            ));
            assert_eq!(p.gesture_count(), 1);
            assert_eq!(
                p.navigate(&r, false),
                NavigationOutcome::Indeterminate(NavigationUncertainty::AlreadyDispatched)
            );
            assert_eq!(p.gesture_count(), 1);
        }
        let mut p = platform();
        let r = next(&p);
        p.inject_navigation_fault(NavigationFault::NoMovement);
        let NavigationOutcome::SyntheticUnchanged(receipt) = p.navigate(&r, false) else {
            panic!("unchanged")
        };
        assert_eq!(receipt.observed(), r.source());
        assert_eq!(p.gesture_count(), 1);
        assert!(matches!(
            p.navigate(&r, false),
            NavigationOutcome::Indeterminate(_)
        ));
        assert_eq!(p.gesture_count(), 1);
    }
    #[test]
    fn creation_handoff_allows_only_explicit_source_or_new_target_visit_once() {
        for selected in [false, true] {
            let mut p = platform();
            p.set_creation_selects_target(selected);
            let receipt = create(&mut p);
            let CreationHandoffOutcome::SyntheticAcquired(observation) =
                p.acquire_after_creation(&receipt, false)
            else {
                panic!("handoff")
            };
            assert_eq!(
                observation.page(),
                if selected {
                    receipt.target()
                } else {
                    receipt.request().source().page()
                }
            );
            assert_eq!(
                observation.visit,
                receipt.request.source.visit + u64::from(selected)
            );
            assert_eq!(
                p.acquire_after_creation(&receipt, false),
                CreationHandoffOutcome::Rejected(RejectionReason::StaleGuard)
            );
            assert_eq!(p.gesture_count(), 0);
        }
    }
    #[test]
    fn changed_creation_guard_and_foreign_receipts_never_refresh_handoff() {
        for fault in 0..7 {
            let mut p = platform();
            let receipt = create(&mut p);
            match fault {
                0 => p.state.input_epoch += 1,
                1 => p.state.session += 1,
                2 => p.state.revision += 1,
                3 => p.state.order.reverse(),
                4 => p.state.visit += 1,
                5 => p.state.page.page = id(5),
                _ => {
                    p.state.page = receipt.target;
                    p.state.visit += 2;
                }
            }
            assert_eq!(
                p.acquire_after_creation(&receipt, false),
                CreationHandoffOutcome::Rejected(RejectionReason::StaleGuard)
            );
            p.state = receipt.request.source.clone();
            p.state.order = receipt.after_order.clone();
            p.state.revision = receipt.revision;
            assert_eq!(
                p.acquire_after_creation(&receipt, false),
                CreationHandoffOutcome::Rejected(RejectionReason::StaleGuard)
            );
        }
        let mut p = platform();
        let receipt = create(&mut p);
        let mut foreign = platform();
        assert_eq!(
            foreign.acquire_after_creation(&receipt, false),
            CreationHandoffOutcome::Rejected(RejectionReason::ForeignScope)
        );
        let mut forged = receipt.clone();
        forged.target.page = id(8);
        assert_eq!(
            p.acquire_after_creation(&forged, false),
            CreationHandoffOutcome::Rejected(RejectionReason::OperationConflict)
        );
        assert_eq!(
            p.acquire_after_creation(&receipt, true),
            CreationHandoffOutcome::CanceledAfterCreation
        );
        assert_eq!(
            p.acquire_after_creation(&receipt, false),
            CreationHandoffOutcome::Rejected(RejectionReason::StaleGuard)
        );
        assert_eq!(p.pages().len(), 4);
        assert_eq!(p.gesture_count(), 0);
    }
    #[test]
    fn native_defaults_and_unqualified_mock_cannot_dispatch_or_handoff() {
        let mut p = platform();
        let request = next(&p);
        let receipt = create(&mut p);
        let mut native = UnqualifiedPlatform;
        assert_eq!(
            native.navigate(&request, false),
            NavigationOutcome::Unsupported(UnsupportedReason::CapabilityNotImplemented)
        );
        assert_eq!(
            native.acquire_after_creation(&receipt, false),
            CreationHandoffOutcome::Unsupported(UnsupportedReason::CapabilityNotImplemented)
        );
        p.set_guard_enforcement(false);
        assert_eq!(
            p.navigate(&request, false),
            NavigationOutcome::Unsupported(UnsupportedReason::UnqualifiedMechanism)
        );
        assert_eq!(
            p.acquire_after_creation(&receipt, false),
            CreationHandoffOutcome::Unsupported(UnsupportedReason::UnqualifiedMechanism)
        );
        assert_eq!(p.gesture_count(), 0);
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
            "00000000-0000-0000-0000-000000000000",
            "../page",
            "12345678-1234-1234-ABCD-123456789ABC",
            "12345678/1234-1234-abcd-123456789abc",
        ] {
            assert!(Uuid::parse(bad).is_none());
        }
    }

    #[test]
    fn unknown_identity_is_not_unsupported_or_a_fabricated_page() {
        let empty = MockPlatform::new(id(1), id(2), vec![], id(3));
        assert_eq!(
            empty.observe_page(),
            Err(ObservationFailure::UnknownIdentity(
                UnknownIdentityReason::NoDocumentOpen
            ))
        );
        let duplicate = MockPlatform::new(id(1), id(2), vec![id(3), id(3)], id(3));
        assert_eq!(
            duplicate.observe_page(),
            Err(ObservationFailure::UnknownIdentity(
                UnknownIdentityReason::AmbiguousIdentity
            ))
        );
        let stale = MockPlatform::new(id(1), id(2), vec![id(4)], id(3));
        assert_eq!(
            stale.observe_page(),
            Err(ObservationFailure::UnknownIdentity(
                UnknownIdentityReason::Unsettled
            ))
        );
        assert_eq!(
            UnqualifiedPlatform.observe_page(),
            Err(ObservationFailure::Unsupported(
                UnsupportedReason::NoNativeAdapter
            ))
        );
        // SDK has no conversation registry: a valid page is known regardless
        // of whether the consumer has ever bound it to a conversation.
        assert_eq!(platform().observe_page().unwrap().page().page, id(3));
    }
}
