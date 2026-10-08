#![cfg(feature = "development-capture")]
use remarkable_open_sdk::development_capture::*;
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::io::Cursor;

fn sample() -> (ExpectedCaptureBinding, Value, Vec<u8>) {
    let expected = ExpectedCaptureBinding {
        nonce: "0123456789abcdef0123456789abcdef".into(),
        attempt_pid: "42".into(),
        attempt_start: "100".into(),
        root_device: "19".into(),
        root_inode: "200".into(),
        document_id: "00000000-0000-4000-8000-000000000001".into(),
        expected_order: vec!["00000000-0000-4000-8000-000000000002".into()],
    };
    let mut png = Cursor::new(Vec::new());
    image::DynamicImage::new_rgba8(2, 3)
        .write_to(&mut png, image::ImageFormat::Png)
        .unwrap();
    let png = png.into_inner();
    let digest: String = Sha256::digest(&png)
        .iter()
        .map(|b| format!("{b:02x}"))
        .collect();
    let receipt = json!({
        "kind":"development-capture-observation", "version":2,
        "nonce":expected.nonce, "attempt_pid":expected.attempt_pid,
        "attempt_start":expected.attempt_start, "root_device":expected.root_device,
        "root_inode":expected.root_inode, "setup_profile":"main-dev-facts-120s",
        "setup_budget_ms":120000, "capture_budget_ms":5000,
        "accepted_ms":100, "baseline_ms":101, "grab_start_ms":102,
        "grab_end_ms":103, "post_read_ms":104, "completed_ms":105,
        "document_id":expected.document_id, "page_id":expected.expected_order[0], "page_index":0,
        "begin_epoch":"1", "end_epoch":"1", "width":2, "height":3, "dpr":1,
        "image_width":2, "image_height":3, "png_bytes":png.len(), "png_sha256":digest,
        "image_status":"available", "gui_callback_completed":true, "scope_current":true,
        "atomic_snapshot":false, "native_authority":false, "render_authority":false,
        "ui_acknowledged":false, "observed_order":false, "discovery_scope":DISCOVERY_SCOPE
    });
    (expected, receipt, png)
}

#[test]
fn owns_exact_bytes_without_promoting_receipt_assertions() {
    let (expected, receipt, png) = sample();
    let raw = serde_json::to_vec_pretty(&receipt).unwrap();
    let capture =
        ReadOnlyDevelopmentCapture::from_collected_v11(&expected, raw.clone(), png.clone())
            .unwrap();
    assert_eq!(capture.png_bytes(), png);
    assert_eq!(capture.completion_bytes(), raw);
    assert_eq!(capture.expected_binding(), &expected);
    assert_eq!(capture.dimensions(), [2, 3]);
    assert_eq!(
        capture.qualification(),
        "DevelopmentUnqualifiedHistoricalCorrespondence"
    );
}

#[test]
fn rejects_wrong_binding_bytes_authority_and_wire_shape() {
    let (expected, receipt, png) = sample();
    let raw = serde_json::to_vec(&receipt).unwrap();
    let mut wrong = expected.clone();
    wrong.attempt_start = "101".into();
    assert!(matches!(
        ReadOnlyDevelopmentCapture::from_collected_v11(&wrong, raw.clone(), png.clone()),
        Err(DevelopmentCaptureError::Binding)
    ));
    let mut altered = png.clone();
    altered[40] ^= 1;
    assert!(matches!(
        ReadOnlyDevelopmentCapture::from_collected_v11(&expected, raw.clone(), altered),
        Err(DevelopmentCaptureError::Image)
    ));
    for field in [
        "native_authority",
        "render_authority",
        "observed_order",
        "atomic_snapshot",
        "ui_acknowledged",
    ] {
        let mut changed = receipt.clone();
        changed[field] = json!(true);
        assert!(matches!(
            ReadOnlyDevelopmentCapture::from_collected_v11(
                &expected,
                serde_json::to_vec(&changed).unwrap(),
                png.clone()
            ),
            Err(DevelopmentCaptureError::Authority)
        ));
    }
    let mut changed = receipt.clone();
    changed["completed_ms"] = json!(5100);
    assert!(matches!(
        ReadOnlyDevelopmentCapture::from_collected_v11(
            &expected,
            serde_json::to_vec(&changed).unwrap(),
            png.clone()
        ),
        Err(DevelopmentCaptureError::Timing)
    ));
    let mut changed = receipt.clone();
    changed["extra"] = json!(false);
    assert!(matches!(
        ReadOnlyDevelopmentCapture::from_collected_v11(
            &expected,
            serde_json::to_vec(&changed).unwrap(),
            png.clone()
        ),
        Err(DevelopmentCaptureError::Shape)
    ));
    let mut changed = receipt.clone();
    changed.as_object_mut().unwrap().remove("observed_order");
    assert!(matches!(
        ReadOnlyDevelopmentCapture::from_collected_v11(
            &expected,
            serde_json::to_vec(&changed).unwrap(),
            png.clone()
        ),
        Err(DevelopmentCaptureError::Shape)
    ));
    let duplicate = String::from_utf8(raw)
        .unwrap()
        .replacen('{', "{\"version\":2,", 1);
    assert!(matches!(
        ReadOnlyDevelopmentCapture::from_collected_v11(&expected, duplicate.into_bytes(), png),
        Err(DevelopmentCaptureError::Shape)
    ));
}

/// Private bytes never enter Git. This is a replay/consistency check, not fresh acquisition.
#[test]
#[ignore = "requires explicitly supplied private historical capture directory"]
fn private_historical_capture_correspondence() {
    let root = std::path::PathBuf::from(
        std::env::var_os("SDK_DEVELOPMENT_CAPTURE_FIXTURE").expect("fixture directory"),
    );
    let binding: Value =
        serde_json::from_slice(&std::fs::read(root.join("expected-binding.json")).unwrap())
            .unwrap();
    let s = |key: &str| binding[key].as_str().unwrap().to_owned();
    let expected = ExpectedCaptureBinding {
        nonce: s("nonce"),
        attempt_pid: s("attempt_pid"),
        attempt_start: s("attempt_start"),
        root_device: s("root_device"),
        root_inode: s("root_inode"),
        document_id: s("document_id"),
        expected_order: binding["expected_order"]
            .as_array()
            .unwrap()
            .iter()
            .map(|v| v.as_str().unwrap().to_owned())
            .collect(),
    };
    let raw = std::fs::read(root.join("capture-observation-complete.json")).unwrap();
    let png = std::fs::read(root.join("capture-window.png")).unwrap();
    let capture =
        ReadOnlyDevelopmentCapture::from_collected_v11(&expected, raw.clone(), png.clone())
            .unwrap();
    assert_eq!(capture.png_bytes(), png);
    assert_eq!(capture.completion_bytes(), raw);
    assert_eq!(capture.expected_binding(), &expected);
}
