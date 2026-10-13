use remarkable_open_sdk::{capabilities::*, *};

const KEYS: [&str; 7] = [
    "observe_page",
    "capture",
    "navigate",
    "create_after/client-selected",
    "create_after/native-assigned",
    "reconcile_creation",
    "acquire_after_creation",
];

struct DefaultPlatform;
impl Platform for DefaultPlatform {
    fn observe_page(&self) -> Result<PageObservation, ObservationFailure> {
        UnqualifiedPlatform.observe_page()
    }
    fn create_after(&mut self, r: &CreationRequest, c: bool) -> CreationOutcome {
        UnqualifiedPlatform.create_after(r, c)
    }
    fn reconcile_creation(&self, r: &CreationRequest) -> Reconciliation {
        UnqualifiedPlatform.reconcile_creation(r)
    }
}

#[test]
fn unknown_profiles_and_unknown_keys_never_advertise_support() {
    for (report, reason) in [
        (
            UnqualifiedPlatform.capabilities(),
            UnsupportedReason::NoNativeAdapter,
        ),
        (
            DefaultPlatform.capabilities(),
            UnsupportedReason::CapabilityNotImplemented,
        ),
    ] {
        assert_eq!(report.contract_revision(), CONTRACT_REVISION);
        assert_eq!(report.profile(), DeviceProfile::UnknownNative);
        assert_eq!(report.profile_revision(), None);
        for key in KEYS {
            assert_eq!(report.status(key), CapabilityStatus::Unsupported(reason));
        }
        for key in ["", "native-supported", "observe_page/2", "firmware=3.28"] {
            assert_eq!(
                report.status(key),
                CapabilityStatus::Unsupported(UnsupportedReason::CapabilityNotImplemented)
            );
        }
    }
}

#[cfg(feature = "mock")]
fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}

#[cfg(feature = "mock")]
#[test]
fn synthetic_query_preserves_pending_model_work_and_distinguishes_capture() {
    use mock::{ExecutionEvent, MockPlatform};
    let mut p = MockPlatform::new(id(1), id(2), vec![id(3), id(4)], id(3));
    p.assign_next_native_id(id(5));
    p.inject_at_execution(ExecutionEvent::CommitThenLoseReply);
    let before = p.observe_page().unwrap();
    let report = p.capabilities();
    assert_eq!(report, p.capabilities());
    assert_eq!(p.observe_page().unwrap(), before);
    assert_eq!(p.gesture_count(), 0);
    assert_eq!(report.profile(), DeviceProfile::SyntheticModel);
    assert_eq!(report.profile_revision(), Some(SYNTHETIC_PROFILE_REVISION));
    for key in KEYS {
        assert_eq!(
            report.status(key),
            if key == "capture" {
                CapabilityStatus::Unsupported(UnsupportedReason::CapabilityNotImplemented)
            } else {
                CapabilityStatus::SyntheticOnly
            }
        );
    }
    assert_eq!(
        report.status("unknown"),
        CapabilityStatus::Unsupported(UnsupportedReason::CapabilityNotImplemented)
    );
    let request =
        CreationRequest::new(OperationId(id(9)), before, TargetAllocation::NativeAssigned);
    assert!(matches!(
        p.create_after(&request, false),
        CreationOutcome::Indeterminate { .. }
    ));
    let Reconciliation::Historical { receipt, .. } = p.reconcile_creation(&request) else {
        panic!()
    };
    assert_eq!(receipt.target().page, id(5));
    assert_eq!(receipt.origin(), EvidenceOrigin::Synthetic);
}

#[cfg(feature = "mock")]
#[test]
fn earlier_synthetic_report_cannot_override_later_refusal_or_native_default() {
    let mut p = mock::MockPlatform::new(id(1), id(2), vec![id(3), id(4)], id(3));
    let request = CreationRequest::new(
        OperationId(id(9)),
        p.observe_page().unwrap(),
        TargetAllocation::ClientSelected(id(5)),
    );
    let old = p.capabilities();
    p.set_guard_enforcement(false);
    for key in [
        "navigate",
        "create_after/client-selected",
        "create_after/native-assigned",
        "acquire_after_creation",
    ] {
        assert_eq!(old.status(key), CapabilityStatus::SyntheticOnly);
        assert_eq!(
            p.capabilities().status(key),
            CapabilityStatus::Unsupported(UnsupportedReason::UnqualifiedMechanism)
        );
    }
    assert_eq!(
        p.capabilities().status("observe_page"),
        CapabilityStatus::SyntheticOnly
    );
    assert_eq!(
        p.capabilities().status("reconcile_creation"),
        CapabilityStatus::SyntheticOnly
    );
    assert_eq!(
        p.create_after(&request, false),
        CreationOutcome::Unsupported(UnsupportedReason::UnqualifiedMechanism)
    );
    assert_eq!(
        UnqualifiedPlatform.create_after(&request, false),
        CreationOutcome::Unsupported(UnsupportedReason::NoNativeAdapter)
    );
    assert_eq!(p.pages(), &[id(3), id(4)]);
}
