#![cfg(feature = "native-adapter")]
use remarkable_open_sdk::{Uuid, native_adapter::ActivationAdmission};

fn admission() -> ActivationAdmission {
    ActivationAdmission {
        profile_sha256: [1; 32],
        executable_sha256: [2; 32],
        adapter_sha256: [3; 32],
        boot: Uuid::parse("11111111-2222-3333-4444-555555555555").unwrap(),
        activation: Uuid::parse("66666666-7777-8888-9999-aaaaaaaaaaaa").unwrap(),
        supervisor_uid: 0,
        supervisor_pid: 0x01020304,
        supervisor_start_ticks: 0x0102030405060708,
        expires_boot_ms: 65_000,
        operation_budget_ms: 30_000,
    }
}

#[test]
fn canonical_bootstrap_layout_and_launch_bounds() {
    let source = admission();
    let bytes = source.encode(5_000).unwrap();
    assert_eq!(&bytes[..8], b"RSDKACT1");
    assert_eq!(&bytes[8..40], &[1; 32]);
    assert_eq!(&bytes[40..72], &[2; 32]);
    assert_eq!(&bytes[72..104], &[3; 32]);
    assert_eq!(&bytes[104..120], source.boot.as_bytes());
    assert_eq!(&bytes[120..136], source.activation.as_bytes());
    assert_eq!(&bytes[136..144], &[0, 0, 0, 0, 1, 2, 3, 4]);
    assert_eq!(&bytes[144..152], &[1, 2, 3, 4, 5, 6, 7, 8]);
    assert_eq!(&bytes[152..160], &[0, 0, 0, 0, 0, 0, 0xfd, 0xe8]);
    assert_eq!(&bytes[160..168], &[0, 0, 0x75, 0x30, 0, 0, 0, 0]);
    assert!(source.encode(0).is_err());
    assert!(source.encode(4_999).is_err());
    assert!(source.encode(65_000).is_err());
    assert!(source.encode(u64::MAX).is_err());
    for invalid in 0..9 {
        let mut bad = admission();
        match invalid {
            0 => bad.profile_sha256 = [0; 32],
            1 => bad.executable_sha256 = [0; 32],
            2 => bad.adapter_sha256 = [0; 32],
            3 => bad.supervisor_pid = 0,
            4 => bad.supervisor_pid = 1,
            5 => bad.supervisor_start_ticks = 0,
            6 => bad.operation_budget_ms = 0,
            7 => bad.operation_budget_ms = 30_001,
            _ => bad.expires_boot_ms = 0,
        }
        assert!(bad.encode(5_000).is_err());
    }
}
