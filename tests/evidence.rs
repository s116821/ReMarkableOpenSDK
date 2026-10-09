#![cfg(feature = "mock")]
use remarkable_open_sdk::{
    capture::{synthetic::*, *},
    evidence::*,
    mock::MockPlatform,
    *,
};
use std::{io::Cursor, time::Duration};

fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("{n:08x}-0000-0000-0000-000000000001")).unwrap()
}
fn fixture(procedure: String) -> CapturedBatch {
    let source = MockPlatform::new(id(1), id(2), vec![id(3)], id(3))
        .observe_page()
        .unwrap();
    let png = {
        let mut bytes = Cursor::new(Vec::new());
        image::DynamicImage::ImageRgba8(image::RgbaImage::from_pixel(
            2,
            2,
            image::Rgba([1, 2, 3, 255]),
        ))
        .write_to(&mut bytes, image::ImageFormat::Png)
        .unwrap();
        bytes.into_inner()
    };
    let request = CaptureRequest::new(
        OperationId(id(4)),
        source.clone(),
        Duration::new(u64::MAX, 0),
        false,
    );
    let acquisition = Acquisition {
        before: source.clone(),
        after: source.clone(),
        rendered_owner: source,
        render_generation: u64::MAX,
        buffer_generation: u64::MAX,
        input_changed: false,
        interval: [
            Duration::new((1u64 << 53) + 1, 123_456_789),
            Duration::new(u64::MAX - 1, 999_999_999),
        ],
        viewport_revision: "v".into(),
        conversion_procedure: "c".into(),
        procedure_revision: procedure,
        native_png: png.clone(),
        native_transform: Affine::new([0.5, -0.0, 0.0, 0.5, -0.0, 0.0]).unwrap(),
        valid_source_region: SourceRegion {
            x: -0.0,
            y: 0.0,
            width: 2.0,
            height: 2.0,
        },
        target: Some([-0.0, 0.5]),
    };
    validate_batch(
        &request,
        acquisition,
        vec![ImageInput {
            role: ImageRole::Overview,
            crop: PixelRect {
                x: 0,
                y: 0,
                width: 2,
                height: 2,
            },
            output_dimensions: [2, 2],
            filter: ResizeFilter::Nearest,
            png,
        }],
        CaptureLimits::default(),
    )
    .unwrap()
}

#[test]
fn canonical_uuid_output_round_trips_bytes_and_text() {
    let text = "01234567-89ab-cdef-0123-456789abcdef";
    let value = Uuid::parse(text).unwrap();
    assert_eq!(value.to_string(), text);
    assert_eq!(
        value.as_bytes(),
        &[
            1, 35, 69, 103, 137, 171, 205, 239, 1, 35, 69, 103, 137, 171, 205, 239
        ]
    );
    assert_eq!(Uuid::parse(&value.to_string()), Some(value));
}

#[test]
fn capture_facts_preserve_geometry_bits_clock_and_every_descriptor() {
    let batch = fixture("p".into());
    let facts = batch.export_facts(ExportLimits::default()).unwrap();
    assert_eq!(facts.schema(), SCHEMA);
    assert_eq!(facts.qualification(), "UnqualifiedSyntheticModel");
    assert_eq!(facts.clock_kind(), "SyntheticInjectedDuration");
    assert_eq!(facts.render_binding(), "SyntheticEqualityAssertion");
    assert_eq!(facts.operation(), id(4));
    assert_eq!(
        facts.source(),
        &batch
            .source()
            .export_facts(ExportLimits::default())
            .unwrap()
    );
    assert_eq!(facts.render_generation(), u64::MAX);
    assert_eq!(facts.buffer_generation(), u64::MAX);
    assert_eq!(facts.interval()[0].seconds(), (1u64 << 53) + 1);
    assert_eq!(facts.interval()[0].nanoseconds(), 123_456_789);
    assert_eq!(facts.interval()[1].seconds(), u64::MAX - 1);
    assert_eq!(facts.interval()[1].nanoseconds(), 999_999_999);
    assert_eq!(
        facts.target_bits(),
        Some([(-0.0f64).to_bits(), 0.5f64.to_bits()])
    );
    assert_eq!(facts.native_parent().affine_bits()[1], (-0.0f64).to_bits());
    assert_eq!(
        facts.native_parent().valid_region_bits()[0],
        (-0.0f64).to_bits()
    );
    for (saved, original) in std::iter::once(facts.native_parent())
        .chain(facts.images())
        .zip(std::iter::once(batch.native_parent()).chain(batch.images()))
    {
        assert_eq!(saved.sha256(), *original.sha256());
        assert_eq!(saved.bytes(), original.bytes().len() as u64);
        assert_eq!(saved.mime(), original.mime());
        assert_eq!(saved.dimensions(), original.dimensions());
        assert_eq!(saved.role(), original.role());
        assert_eq!(
            saved.affine_bits(),
            original.transform().coefficients().map(f64::to_bits)
        );
        // One possible consumer representation: fixed hex preserves each bit.
        for bits in saved.affine_bits() {
            assert_eq!(
                u64::from_str_radix(&format!("{bits:016x}"), 16).unwrap(),
                bits
            );
        }
    }
    let lineage = facts.images()[0].derivation().unwrap();
    assert_eq!(lineage.parent_digest(), facts.native_parent().sha256());
    assert_eq!(lineage.parent_dimensions(), [2, 2]);
    assert_eq!(lineage.crop(), [0, 0, 2, 2]);
    assert_eq!(lineage.output_dimensions(), [2, 2]);
    assert_eq!(lineage.filter(), ResizeFilter::Nearest);
    assert_eq!(
        lineage.procedure(),
        batch.images()[0].derivation().unwrap().procedure
    );
    assert_eq!(facts.viewport_revision(), "v");
    assert_eq!(facts.conversion_procedure(), "c");
    assert_eq!(facts.procedure_revision(), "p");
    assert_eq!(batch.native_parent().bytes(), batch.images()[0].bytes());
}

#[test]
fn all_copied_string_and_order_occurrences_consume_the_export_budget() {
    let batch = fixture("p".into());
    let exact = 16 + 3 + batch.images()[0].derivation().unwrap().procedure.len();
    assert!(matches!(
        batch.export_facts(ExportLimits {
            max_variable_bytes: exact - 1,
            ..ExportLimits::default()
        }),
        Err(ExportError::MetadataBound)
    ));
    assert!(
        batch
            .export_facts(ExportLimits {
                max_variable_bytes: exact,
                ..ExportLimits::default()
            })
            .is_ok()
    );
    assert!(matches!(
        batch.export_facts(ExportLimits {
            max_images: 1,
            ..ExportLimits::default()
        }),
        Err(ExportError::ImageBound)
    ));
    assert!(matches!(
        fixture("x".repeat(4097)).export_facts(ExportLimits::default()),
        Err(ExportError::StringBound)
    ));
    assert!(matches!(
        batch.export_facts(ExportLimits {
            max_string_bytes: 1,
            ..ExportLimits::default()
        }),
        Err(ExportError::StringBound)
    ));
    assert_eq!(origin_name(EvidenceOrigin::Synthetic), "Synthetic");
    assert_eq!(role_name(ImageRole::Detail(7)), "Detail");
    assert_eq!(
        allocation_name(TargetAllocation::NativeAssigned),
        "NativeAssigned"
    );
    assert_eq!(filter_name(ResizeFilter::Triangle), "Triangle");
}
