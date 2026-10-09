//! Ordinary gesture geometry, not native observation or completion authority.
use super::LogicalDirection;
use std::time::Duration;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum TabletModel {
    Remarkable1,
    Remarkable2,
    PaperPro,
    Unknown,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Orientation {
    Portrait,
    PortraitInverted,
    LandscapeLeft,
    LandscapeRight,
    Unknown,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum UnsupportedGesture {
    UnknownTablet,
    UnknownOrientation,
    UnimplementedPair,
}

/// Coordinates in Reader's existing 768 by 1024 virtual input space.
/// This value cannot authorize dispatch or establish page identity/completion.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct GesturePlan {
    start: (i32, i32),
    movements: [(i32, i32); 15],
}

impl GesturePlan {
    pub fn start(&self) -> (i32, i32) {
        self.start
    }

    pub fn movements(&self) -> &[(i32, i32); 15] {
        &self.movements
    }

    pub fn hold_duration(&self) -> Duration {
        Duration::from_millis(50)
    }

    pub fn movement_interval(&self) -> Duration {
        Duration::from_millis(10)
    }
}

/// Caller must establish the explicit tablet/orientation for the current session.
/// No unknown input defaults to RM2, and no orientation is inferred here.
pub fn plan_gesture(
    tablet: TabletModel,
    orientation: Orientation,
    direction: LogicalDirection,
) -> Result<GesturePlan, UnsupportedGesture> {
    if tablet == TabletModel::Unknown {
        return Err(UnsupportedGesture::UnknownTablet);
    }
    if orientation == Orientation::Unknown {
        return Err(UnsupportedGesture::UnknownOrientation);
    }
    if (tablet, orientation) != (TabletModel::Remarkable2, Orientation::Portrait) {
        return Err(UnsupportedGesture::UnimplementedPair);
    }
    let (start_x, end_x): (i32, i32) = match direction {
        LogicalDirection::Next => (700, 100),
        LogicalDirection::Previous => (100, 700),
    };
    let movements = std::array::from_fn(|index| {
        // Preserve Reader's per-direction floating-point truncation exactly.
        let offset = ((end_x - start_x).abs() as f32 * ((index + 1) as f32 / 15.0)) as i32;
        (start_x + (end_x - start_x).signum() * offset, 512)
    });
    Ok(GesturePlan {
        start: (start_x, 512),
        movements,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn both_directions_preserve_every_existing_reader_step_and_timing() {
        for (direction, start, expected_x) in [
            (
                LogicalDirection::Next,
                700,
                [
                    660, 620, 580, 540, 500, 460, 420, 380, 340, 300, 260, 220, 180, 140, 100,
                ],
            ),
            (
                LogicalDirection::Previous,
                100,
                [
                    140, 180, 220, 260, 300, 340, 380, 420, 460, 500, 540, 580, 620, 660, 700,
                ],
            ),
        ] {
            let plan =
                plan_gesture(TabletModel::Remarkable2, Orientation::Portrait, direction).unwrap();
            assert_eq!(plan.start(), (start, 512));
            assert_eq!(*plan.movements(), expected_x.map(|x| (x, 512)));
            assert_eq!(plan.hold_duration(), Duration::from_millis(50));
            assert_eq!(plan.movement_interval(), Duration::from_millis(10));
        }
    }

    #[test]
    fn unknown_and_unimplemented_pairs_never_produce_geometry() {
        for direction in [LogicalDirection::Next, LogicalDirection::Previous] {
            assert_eq!(
                plan_gesture(TabletModel::Unknown, Orientation::Portrait, direction),
                Err(UnsupportedGesture::UnknownTablet)
            );
            assert_eq!(
                plan_gesture(TabletModel::Remarkable2, Orientation::Unknown, direction),
                Err(UnsupportedGesture::UnknownOrientation)
            );
            for (tablet, orientation) in [
                (TabletModel::Remarkable1, Orientation::Portrait),
                (TabletModel::PaperPro, Orientation::Portrait),
                (TabletModel::Remarkable2, Orientation::PortraitInverted),
                (TabletModel::Remarkable2, Orientation::LandscapeLeft),
                (TabletModel::Remarkable2, Orientation::LandscapeRight),
            ] {
                assert_eq!(
                    plan_gesture(tablet, orientation, direction),
                    Err(UnsupportedGesture::UnimplementedPair)
                );
            }
        }
    }
}
