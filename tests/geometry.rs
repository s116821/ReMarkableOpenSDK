use remarkable_open_sdk::capture::{Affine, PixelRect, SourceRegion, derive_geometry};

fn region(x: f64, y: f64, width: f64, height: f64) -> SourceRegion {
    SourceRegion {
        x,
        y,
        width,
        height,
    }
}
fn crop(x: u32, y: u32, width: u32, height: u32) -> PixelRect {
    PixelRect {
        x,
        y,
        width,
        height,
    }
}

#[test]
fn clipped_resized_crop_has_exact_composed_geometry() {
    let affine = Affine::new([0.01, 0.0, 0.0, 0.01, 0.0, 0.0]).unwrap();
    let geometry = derive_geometry(
        [100, 100],
        affine,
        region(10.0, 20.0, 70.0, 60.0),
        crop(0, 30, 60, 60),
        [30, 20],
    )
    .unwrap();
    assert_eq!(
        geometry.affine().coefficients().map(f64::to_bits),
        [0.02_f64, 0.0, 0.0, 0.03, 0.0, 0.3].map(f64::to_bits)
    );
    assert_eq!(
        geometry.valid_source_region(),
        // The pinned procedure multiplies by the scale; direct division rounds differently.
        region(5.0, 0.0, 25.0, 16.666_666_666_666_664)
    );
    let point = geometry.affine().map([10.0, 10.0]).unwrap();
    assert_eq!(point, affine.map([20.0, 60.0]).unwrap());
}

#[test]
fn dimensions_crop_and_nonintersection_refuse_without_mock_feature() {
    let affine = Affine::new([0.01, 0.0, 0.0, 0.01, 0.0, 0.0]).unwrap();
    let source = region(10.0, 20.0, 70.0, 60.0);
    for (parent, cropped, output) in [
        ([0, 100], crop(0, 0, 100, 100), [10, 10]),
        ([32769, 100], crop(0, 0, 100, 100), [10, 10]),
        ([100, 100], crop(0, 0, 100, 100), [0, 10]),
        ([100, 100], crop(0, 0, 100, 100), [10, 32769]),
        ([100, 100], crop(u32::MAX, 0, 2, 1), [10, 10]),
        ([100, 100], crop(90, 0, 20, 100), [10, 10]),
        ([100, 100], crop(0, 0, 0, 100), [10, 10]),
        ([100, 100], crop(0, 0, 10, 20), [10, 10]),
    ] {
        assert!(derive_geometry(parent, affine, source, cropped, output).is_err());
    }
    for invalid in [
        region(f64::NAN, 0.0, 1.0, 1.0),
        region(-1.0, 0.0, 1.0, 1.0),
        region(0.0, 0.0, 101.0, 1.0),
        region(0.0, 0.0, 1.0, 0.0),
    ] {
        assert!(
            derive_geometry([100, 100], affine, invalid, crop(0, 0, 100, 100), [10, 10]).is_err()
        );
    }
    let outside = Affine::new([0.01, 0.0, 0.0, 0.01, 1.0, 0.0]).unwrap();
    assert!(derive_geometry([100, 100], outside, source, crop(0, 0, 100, 100), [10, 10]).is_err());
}

#[test]
fn rotation_signed_zero_and_source_plane_tolerance_are_distinct() {
    let affine = Affine::new([0.0, 0.01, -0.01, -0.0, 1.0, 0.0]).unwrap();
    let geometry = derive_geometry(
        [100, 100],
        affine,
        region(0.0, 0.0, 100.0, 100.0),
        crop(20, 30, 40, 50),
        [20, 25],
    )
    .unwrap();
    assert_eq!(
        geometry.affine().coefficients().map(f64::to_bits),
        [0.0_f64, 0.02, -0.02, -0.0, 0.7, 0.2].map(f64::to_bits)
    );
    assert_ne!(
        geometry.affine().coefficients()[3].to_bits(),
        0.0_f64.to_bits()
    );
    for (offset, accepted) in [(5e-11, true), (2e-10, false)] {
        let affine = Affine::new([0.01, 0.0, 0.0, 0.01, offset, 0.0]).unwrap();
        let result = derive_geometry(
            [100, 100],
            affine,
            region(0.0, 0.0, 100.0, 100.0),
            crop(0, 0, 100, 100),
            [100, 100],
        );
        assert_eq!(result.is_ok(), accepted);
        if let Ok(geometry) = result {
            assert_eq!(
                geometry.affine().coefficients()[4].to_bits(),
                offset.to_bits()
            );
        }
    }
}
