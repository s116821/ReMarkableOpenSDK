#![cfg(feature = "mock")]
use image::{DynamicImage, ImageEncoder, ImageFormat};
use remarkable_open_sdk::{
    capture::{synthetic::*, *},
    mock::MockPlatform,
    *,
};
use sha2::{Digest, Sha256};
use std::{io::Cursor, time::Duration};

fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("{n:08x}-0000-0000-0000-000000000001")).unwrap()
}
fn png(image: &DynamicImage) -> Vec<u8> {
    let mut output = Cursor::new(Vec::new());
    image.write_to(&mut output, ImageFormat::Png).unwrap();
    output.into_inner()
}
fn pixels() -> DynamicImage {
    DynamicImage::ImageRgba8(image::RgbaImage::from_fn(12, 20, |x, y| {
        image::Rgba([(x * 17) as u8, (y * 11) as u8, 47, 255])
    }))
}
fn setup() -> (CaptureRequest, Acquisition, Vec<ImageInput>) {
    let source = MockPlatform::new(id(1), id(2), vec![id(3)], id(3))
        .observe_page()
        .unwrap();
    let request = CaptureRequest::new(
        OperationId(id(4)),
        source.clone(),
        Duration::from_millis(100),
        false,
    );
    let native = pixels();
    let acquisition = Acquisition {
        before: source.clone(),
        after: source.clone(),
        rendered_owner: source,
        render_generation: 7,
        buffer_generation: 7,
        input_changed: false,
        interval: [Duration::from_millis(1), Duration::from_millis(5)],
        viewport_revision: "synthetic-viewport-1".into(),
        conversion_procedure: "synthetic-rgba8".into(),
        procedure_revision: "synthetic-frame-binding-v1".into(),
        native_png: png(&native),
        native_transform: Affine::new([1.0 / 12.0, 0.0, 0.0, 1.0 / 20.0, 0.0, 0.0]).unwrap(),
        valid_source_region: SourceRegion {
            x: 0.0,
            y: 0.0,
            width: 12.0,
            height: 20.0,
        },
        target: Some([0.5, 0.5]),
    };
    let overview = native.resize_exact(6, 10, image::imageops::FilterType::Nearest);
    let detail = native.crop_imm(0, 6, 12, 8);
    let inputs = vec![
        ImageInput {
            role: ImageRole::Overview,
            crop: PixelRect {
                x: 0,
                y: 0,
                width: 12,
                height: 20,
            },
            output_dimensions: [6, 10],
            filter: ResizeFilter::Nearest,
            png: png(&overview),
        },
        ImageInput {
            role: ImageRole::Detail(0),
            crop: PixelRect {
                x: 0,
                y: 6,
                width: 12,
                height: 8,
            },
            output_dimensions: [12, 8],
            filter: ResizeFilter::Nearest,
            png: png(&detail),
        },
    ];
    (request, acquisition, inputs)
}

#[test]
fn native_parent_and_sibling_derivatives_preserve_exact_encodings_and_hashes() {
    let (request, acquisition, inputs) = setup();
    let parent = acquisition.native_png.clone();
    let overview = inputs[0].png.clone();
    let detail = inputs[1].png.clone();
    let batch = validate_batch(&request, acquisition, inputs, CaptureLimits::default()).unwrap();
    assert_eq!(batch.origin(), EvidenceOrigin::Synthetic);
    assert_eq!(batch.native_parent().bytes(), parent);
    assert_eq!(batch.images()[0].bytes(), overview);
    assert_eq!(batch.images()[1].bytes(), detail);
    assert_eq!(
        *batch.native_parent().sha256(),
        <[u8; 32]>::from(Sha256::digest(&parent))
    );
    assert_eq!(batch.images()[1].dimensions(), [12, 8]);
    assert_eq!(batch.images()[0].dimensions(), [6, 10]);
    for image in batch.images() {
        assert_eq!(
            &image.derivation().unwrap().parent_digest,
            batch.native_parent().sha256()
        );
    }
    assert_eq!(
        batch.encoded_bytes(),
        parent.len() + overview.len() + detail.len()
    );
    let clone = batch.clone();
    let mut copy = clone.images()[0].bytes().to_vec();
    copy[0] = 0;
    assert_eq!(batch.images()[0].bytes(), overview);
}

#[test]
fn valid_but_unrelated_detail_pixels_cannot_borrow_plausible_crop_metadata() {
    let (request, acquisition, mut inputs) = setup();
    let wrong = DynamicImage::ImageRgba8(image::RgbaImage::from_pixel(
        12,
        8,
        image::Rgba([0, 0, 0, 255]),
    ));
    inputs[1].png = png(&wrong);
    assert!(matches!(
        validate_batch(&request, acquisition, inputs, CaptureLimits::default()),
        Err(CaptureFailure::PixelMismatch)
    ));
}

#[test]
fn equivalent_lossless_png_encoding_is_verified_and_retained_without_reencoding() {
    let (request, acquisition, mut inputs) = setup();
    let expected = pixels().crop_imm(0, 6, 12, 8).to_rgba8();
    let mut alternate = vec![];
    image::codecs::png::PngEncoder::new_with_quality(
        &mut alternate,
        image::codecs::png::CompressionType::Best,
        image::codecs::png::FilterType::Paeth,
    )
    .write_image(expected.as_raw(), 12, 8, image::ExtendedColorType::Rgba8)
    .unwrap();
    assert_ne!(alternate, inputs[1].png);
    inputs[1].png = alternate.clone();
    let batch = validate_batch(&request, acquisition, inputs, CaptureLimits::default()).unwrap();
    assert_eq!(batch.images()[1].bytes(), alternate);
}

#[test]
fn stable_owner_with_stale_buffer_and_away_back_input_are_refused() {
    for mutation in 0..4 {
        let (request, mut acquisition, inputs) = setup();
        let expected = match mutation {
            0 => {
                acquisition.buffer_generation = 6;
                CaptureFailure::StaleRender
            }
            1 => {
                acquisition.rendered_owner = MockPlatform::new(id(1), id(2), vec![id(8)], id(8))
                    .observe_page()
                    .unwrap();
                CaptureFailure::StaleRender
            }
            2 => {
                acquisition.input_changed = true;
                CaptureFailure::InputChanged
            }
            _ => {
                acquisition.after = MockPlatform::new(id(1), id(2), vec![id(3)], id(3))
                    .observe_page()
                    .unwrap();
                CaptureFailure::OwnerChanged
            }
        };
        assert!(
            matches!(validate_batch(&request,acquisition,inputs,CaptureLimits::default()),Err(reason) if reason==expected)
        );
    }
}

#[test]
fn encoded_decoded_and_batch_bounds_refuse_before_pixel_derivation() {
    let (request, mut acquisition, inputs) = setup();
    acquisition.native_png[16..20].copy_from_slice(&u32::MAX.to_be_bytes());
    assert!(matches!(
        validate_batch(&request, acquisition, inputs, CaptureLimits::default()),
        Err(CaptureFailure::DecodedBound)
    ));
    let (request, acquisition, inputs) = setup();
    let limits = CaptureLimits {
        max_encoded_image_bytes: 1,
        max_encoded_batch_bytes: 1,
        ..CaptureLimits::default()
    };
    assert!(matches!(
        validate_batch(&request, acquisition, inputs, limits),
        Err(CaptureFailure::EncodedBound)
    ));
    let (request, acquisition, inputs) = setup();
    let limits = CaptureLimits {
        max_images: 2,
        ..CaptureLimits::default()
    };
    assert!(matches!(
        validate_batch(&request, acquisition, inputs, limits),
        Err(CaptureFailure::EncodedBound)
    ));
}

#[test]
fn malformed_encoding_overflow_crop_duplicate_role_and_empty_page_region_refuse() {
    for mutation in 0..5 {
        let (request, mut acquisition, mut inputs) = setup();
        match mutation {
            0 => inputs[1].png = b"not a PNG".to_vec(),
            1 => inputs[1].crop.x = u32::MAX,
            2 => inputs[1].role = ImageRole::Overview,
            3 => acquisition.native_png[24] = 16,
            _ => acquisition.valid_source_region.width = 0.0,
        }
        assert!(validate_batch(&request, acquisition, inputs, CaptureLimits::default()).is_err());
    }
    assert!(Affine::new([0.0; 6]).is_err());
    assert!(Affine::new([f64::NAN, 0.0, 0.0, 1.0, 0.0, 0.0]).is_err());
}

#[test]
fn rotated_scrolled_and_chrome_excluded_geometry_is_composed_from_native_parent() {
    let (request, mut acquisition, inputs) = setup();
    acquisition.native_transform =
        Affine::new([0.0, 1.0 / 12.0, -1.0 / 20.0, 0.0, 1.0, 0.0]).unwrap();
    let batch = validate_batch(&request, acquisition, inputs, CaptureLimits::default()).unwrap();
    let point = batch.images()[1].transform().map([0.0, 0.0]).unwrap();
    assert!((point[0] - 0.7).abs() < 1e-12);
    assert_eq!(point[1], 0.0);
    let (request, mut acquisition, inputs) = setup();
    acquisition.native_transform =
        Affine::new([1.0 / 12.0, 0.0, 0.0, 0.5 / 20.0, 0.0, 0.25]).unwrap();
    let batch = validate_batch(&request, acquisition, inputs, CaptureLimits::default()).unwrap();
    assert!((batch.images()[1].transform().map([0.0, 0.0]).unwrap()[1] - 0.4).abs() < 1e-12);
    let (request, mut acquisition, inputs) = setup();
    acquisition.valid_source_region = SourceRegion {
        x: 2.0,
        y: 3.0,
        width: 8.0,
        height: 14.0,
    };
    acquisition.native_transform =
        Affine::new([1.0 / 8.0, 0.0, 0.0, 1.0 / 14.0, -2.0 / 8.0, -3.0 / 14.0]).unwrap();
    let batch = validate_batch(&request, acquisition, inputs, CaptureLimits::default()).unwrap();
    assert_eq!(
        batch.images()[0].valid_source_region(),
        SourceRegion {
            x: 1.0,
            y: 1.5,
            width: 4.0,
            height: 7.0
        }
    );
}

#[test]
fn outside_viewport_target_deadline_and_cancellation_never_return_a_batch() {
    let (request, mut acquisition, inputs) = setup();
    acquisition.native_transform =
        Affine::new([1.0 / 12.0, 0.0, 0.0, 0.5 / 20.0, 0.0, 0.25]).unwrap();
    acquisition.target = Some([0.5, 0.9]);
    assert!(matches!(
        validate_batch(&request, acquisition, inputs, CaptureLimits::default()),
        Err(CaptureFailure::InvalidGeometry)
    ));
    let (request, mut acquisition, inputs) = setup();
    acquisition.interval[1] = request.deadline();
    assert!(matches!(
        validate_batch(&request, acquisition, inputs, CaptureLimits::default()),
        Err(CaptureFailure::DeadlineExpired)
    ));
    let (request, acquisition, inputs) = setup();
    let canceled = CaptureRequest::new(
        request.operation(),
        request.source().clone(),
        request.deadline(),
        true,
    );
    assert!(matches!(
        validate_batch(&canceled, acquisition, inputs, CaptureLimits::default()),
        Err(CaptureFailure::Canceled)
    ));
    let (request, _, _) = setup();
    assert!(matches!(
        UnqualifiedPlatform.capture(&request),
        CaptureOutcome::Unsupported(_)
    ));
}
