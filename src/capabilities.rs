//! Observational implementation availability, never admission or native authority.
use crate::UnsupportedReason;

/// Semantic schema identity, independent of package version and release tags.
pub const CONTRACT_REVISION: &str = "remarkable-open-sdk/semantic-contract/1";
pub const SYNTHETIC_PROFILE_REVISION: &str = "remarkable-open-sdk/synthetic-model/1";

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum DeviceProfile {
    /// No qualified native profile or firmware identity is known.
    UnknownNative,
    /// An in-memory model, not a hardware/firmware compatibility claim.
    SyntheticModel,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CapabilityStatus {
    Unsupported(UnsupportedReason),
    /// Models this operation; its call-time prerequisites can still refuse it.
    SyntheticOnly,
}

/// Immutable snapshot. No operation accepts this value as a guard or receipt.
/// Query again to inspect changed availability; neither report admits execution.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct CapabilityReport {
    profile: DeviceProfile,
    unavailable: UnsupportedReason,
    guarded_model_operations: bool,
}

impl CapabilityReport {
    pub(crate) fn unsupported(reason: UnsupportedReason) -> Self {
        Self {
            profile: DeviceProfile::UnknownNative,
            unavailable: reason,
            guarded_model_operations: false,
        }
    }

    #[cfg(any(test, feature = "mock"))]
    pub(crate) fn synthetic(guarded_model_operations: bool) -> Self {
        Self {
            profile: DeviceProfile::SyntheticModel,
            unavailable: UnsupportedReason::UnqualifiedMechanism,
            guarded_model_operations,
        }
    }

    pub fn contract_revision(&self) -> &'static str {
        CONTRACT_REVISION
    }

    pub fn profile(&self) -> DeviceProfile {
        self.profile
    }

    pub fn profile_revision(&self) -> Option<&'static str> {
        match self.profile {
            DeviceProfile::UnknownNative => None,
            DeviceProfile::SyntheticModel => Some(SYNTHETIC_PROFILE_REVISION),
        }
    }

    /// Recognized keys: observe_page, capture, navigate, create_after/client-selected,
    /// create_after/native-assigned, reconcile_creation, acquire_after_creation.
    /// Unknown keys are unsupported. Native-assigned modeling does not promise
    /// persistent correlation or an available fixture ID; capture acquisition is
    /// not implemented by the mock even though synthetic batch validation exists.
    pub fn status(&self, key: &str) -> CapabilityStatus {
        use CapabilityStatus::*;
        let guarded = match key {
            "observe_page" | "reconcile_creation" => false,
            "navigate"
            | "create_after/client-selected"
            | "create_after/native-assigned"
            | "acquire_after_creation" => true,
            "capture" => {
                return Unsupported(if self.profile == DeviceProfile::UnknownNative {
                    self.unavailable
                } else {
                    UnsupportedReason::CapabilityNotImplemented
                });
            }
            _ => return Unsupported(UnsupportedReason::CapabilityNotImplemented),
        };
        if self.profile == DeviceProfile::UnknownNative
            || (guarded && !self.guarded_model_operations)
        {
            Unsupported(self.unavailable)
        } else {
            SyntheticOnly
        }
    }
}
