use remarkable_open_sdk::{
    ObservationFailure, PageKey, Platform, UnqualifiedPlatform, UnsupportedReason, Uuid,
    capabilities::{CONTRACT_REVISION, CapabilityStatus, DeviceProfile},
};

fn main() {
    let id = Uuid::parse("00000001-1111-1111-1111-111111111111").unwrap();
    assert_eq!(Uuid::parse(&id.to_string()), Some(id));
    let page = PageKey {
        document: id,
        page: id,
    };
    assert_eq!(page.page.as_bytes(), id.as_bytes());
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
    println!("ordinary-consumer: unsupported native profile; no SDK features");
}
