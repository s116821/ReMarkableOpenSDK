//! Build-owned external consumer of the exact candidate host library.
#![forbid(unsafe_code)]
use remarkable_open_sdk::{
    ObservationFailure, Platform, UnqualifiedPlatform, UnsupportedReason, Uuid,
};

fn main() {
    let args: Vec<_> = std::env::args().skip(1).collect();
    assert_eq!(args, [env!("SDK_PROBE_TAG"), env!("SDK_PROBE_SHA")]);
    let text = "00000001-1111-1111-1111-111111111111";
    assert_eq!(Uuid::parse(text).unwrap().to_string(), text);
    assert!(Uuid::parse("00000000-0000-0000-0000-000000000000").is_none());
    assert_eq!(
        UnqualifiedPlatform.observe_page(),
        Err(ObservationFailure::Unsupported(
            UnsupportedReason::NoNativeAdapter
        ))
    );
    println!(
        "{{\"probe\":\"sdk-consumer\",\"candidate\":\"{}\",\"source\":\"{}\",\"native_operations\":\"unsupported\"}}",
        env!("SDK_PROBE_TAG"), env!("SDK_PROBE_SHA")
    );
}
