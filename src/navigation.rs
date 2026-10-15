//! Ordinary gesture planning; no dispatch or native completion authority.
pub mod gesture;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum LogicalDirection {
    Next,
    Previous,
}
