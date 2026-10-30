use remarkable_open_sdk::{
    ObservationFailure, OperationId, PageKey, Platform, UnqualifiedPlatform, UnsupportedReason,
    Uuid,
    capabilities::{CONTRACT_REVISION, CapabilityStatus, DeviceProfile},
    navigation::{LogicalDirection, gesture::*},
};

fn main() {
    let id = Uuid::parse("00000001-1111-1111-1111-111111111111").unwrap();
    assert_eq!(Uuid::parse(&id.to_string()), Some(id));
    let page = PageKey {
        document: id,
        page: id,
    };
    assert_eq!(page.page.as_bytes(), OperationId(id).0.as_bytes());
    let plan = plan_gesture(
        TabletModel::Remarkable2,
        Orientation::Portrait,
        LogicalDirection::Next,
    )
    .unwrap();
    assert_eq!(plan.start(), (700, 512));
    assert_eq!(plan.movements().last(), Some(&(100, 512)));
    assert_eq!(
        plan_gesture(
            TabletModel::Remarkable1,
            Orientation::Portrait,
            LogicalDirection::Next
        ),
        Err(UnsupportedGesture::UnimplementedPair)
    );
    let platform = UnqualifiedPlatform;
    let report = platform.capabilities();
    assert_eq!(report.contract_revision(), CONTRACT_REVISION);
    assert_eq!(report.profile(), DeviceProfile::UnknownNative);
    assert_eq!(report.profile_revision(), None);
    for key in [
        "observe_page",
        "capture",
        "navigate",
        "create_after/client-selected",
        "create_after/native-assigned",
        "reconcile_creation",
        "acquire_after_creation",
    ] {
        assert_eq!(
            report.status(key),
            CapabilityStatus::Unsupported(UnsupportedReason::NoNativeAdapter)
        );
    }
    assert_eq!(
        platform.observe_page(),
        Err(ObservationFailure::Unsupported(
            UnsupportedReason::NoNativeAdapter
        ))
    );
    println!("ordinary-consumer: identity and pure geometry; no SDK features or native dispatch");
}
