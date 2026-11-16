//! Development-only composer wire facts, not a live edit lease or Submit policy.
//!
//! IDs belong to one native runtime/visit. Raw UTF-16 is preserved, including
//! BOMs and unpaired surrogates. A consumer must durably retain a submission
//! before requesting clear; the native writer independently compares live state.
use super::{ProtocolError, Reader, ReaderFacts, Result, Writer};
use crate::Uuid;

pub const MAX_DRAFT_UNITS: usize = 4096;
pub const MAX_KEY_TEXT_UNITS: usize = 64;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RangeIdentity {
    pub facts: ReaderFacts,
    pub visit: Uuid,
    pub range: Uuid,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RangeState {
    pub identity: RangeIdentity,
    /// Genuine copied native revision. Not a transferable currentness lease.
    pub revision: i32,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Draft {
    pub state: RangeState,
    pub units: Vec<u16>,
}

impl Draft {
    /// Strict conversion for product text; never silently replace malformed units.
    pub fn text(&self) -> std::result::Result<String, std::string::FromUtf16Error> {
        String::from_utf16(&self.units)
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ClearReceipt {
    pub identity: RangeIdentity,
    pub before_revision: i32,
    pub after_revision: i32,
}

/// Copied Qt event fields. Neither modifiers nor native codes identify origin.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct KeyFacts {
    pub key: i32,
    pub count: u16,
    pub modifiers: u32,
    pub native_scan_code: u32,
    pub native_virtual_key: u32,
    pub native_modifiers: u32,
    pub auto_repeat: bool,
    pub spontaneous: bool,
    pub text: Vec<u16>,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct KeyObservation {
    pub draft: Draft,
    pub event: Uuid,
    /// Every KeyPress/InputMethod within this visit contributes to this serial.
    /// A nonzero serial is correspondence, not causality or a Submit decision.
    pub input_serial: u64,
    pub key: KeyFacts,
}

/// Native attempt/completion facts. Completion alone never proves an edit.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Attempts(u8);

impl Attempts {
    pub fn from_bits(bits: u8) -> Result<Self> {
        if !matches!(bits, 0 | 1 | 3 | 5 | 7) {
            return Err(ProtocolError::Malformed);
        }
        Ok(Self(bits))
    }
    pub fn bits(self) -> u8 {
        self.0
    }
    pub fn scene_request_attempted(self) -> bool {
        self.0 & 1 != 0
    }
    pub fn edit_attempted(self) -> bool {
        self.0 & 2 != 0
    }
    pub fn worker_completed(self) -> bool {
        self.0 & 4 != 0
    }
}

/// Only Verified has a payload. Unknown must never issue partial capabilities,
/// imply draft clearing or authorize a retry of the original mutation.
#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Outcome<T> {
    Verified { attempts: Attempts, value: T },
    Refused { attempts: Attempts },
    Unknown { attempts: Attempts },
}

fn valid_header(status: u8, attempts: Attempts, effectful: bool) -> bool {
    match status {
        0 => attempts.0 == if effectful { 7 } else { 5 },
        1 => matches!(attempts.0, 0 | 5),
        2 if effectful => matches!(attempts.0, 1 | 3 | 5 | 7),
        2 => matches!(attempts.0, 1 | 5),
        _ => false,
    }
}

impl<T> Outcome<T> {
    fn header(&self, effectful: bool) -> Result<(u8, Attempts)> {
        let (status, attempts) = match self {
            Self::Verified { attempts, .. } => (0, *attempts),
            Self::Refused { attempts } => (1, *attempts),
            Self::Unknown { attempts } => (2, *attempts),
        };
        if !valid_header(status, attempts, effectful) {
            return Err(ProtocolError::Malformed);
        }
        Ok((status, attempts))
    }

    fn encode(
        &self,
        w: &mut Writer,
        effectful: bool,
        value: impl FnOnce(&mut Writer, &T) -> Result<()>,
    ) -> Result<()> {
        let (status, attempts) = self.header(effectful)?;
        w.u8(status);
        w.u8(attempts.0);
        if let Self::Verified { value: payload, .. } = self {
            value(w, payload)?;
        }
        Ok(())
    }

    fn decode(
        r: &mut Reader<'_>,
        effectful: bool,
        value: impl FnOnce(&mut Reader<'_>) -> Result<T>,
    ) -> Result<Self> {
        let status = r.u8()?;
        let attempts = Attempts::from_bits(r.u8()?)?;
        if !valid_header(status, attempts, effectful) {
            return Err(ProtocolError::Malformed);
        }
        Ok(match status {
            0 => Self::Verified {
                attempts,
                value: value(r)?,
            },
            1 => Self::Refused { attempts },
            2 => Self::Unknown { attempts },
            _ => unreachable!(),
        })
    }

    fn agrees(&self, compare: impl FnOnce(&T) -> bool) -> bool {
        match self {
            Self::Verified { value, .. } => compare(value),
            Self::Refused { .. } | Self::Unknown { .. } => true,
        }
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Command {
    Provision(ReaderFacts),
    Observe(RangeIdentity),
    Clear(Draft),
    AwaitKey {
        identity: RangeIdentity,
        keys: Vec<i32>,
    },
}

fn valid_key(key: i32) -> bool {
    key > 0 && key < 0x01ff_ffff
}

fn valid_keys(keys: &[i32]) -> bool {
    (1..=4).contains(&keys.len())
        && keys
            .iter()
            .enumerate()
            .all(|(i, key)| valid_key(*key) && !keys[..i].contains(key))
}

impl Command {
    pub fn is_effectful(&self) -> bool {
        matches!(self, Self::Provision(_) | Self::Clear(_))
    }

    pub(super) fn encode(&self, w: &mut Writer) -> Result<()> {
        match self {
            Self::Provision(facts) => {
                w.u8(12);
                w.facts(facts);
            }
            Self::Observe(identity) => {
                w.u8(13);
                write_identity(w, identity);
            }
            Self::Clear(expected) => {
                w.u8(14);
                write_draft(w, expected)?;
            }
            Self::AwaitKey { identity, keys } => {
                if !valid_keys(keys) {
                    return Err(ProtocolError::Malformed);
                }
                w.u8(15);
                write_identity(w, identity);
                w.u8(keys.len() as u8);
                for key in keys {
                    w.u32(*key as u32);
                }
            }
        }
        Ok(())
    }

    pub(super) fn decode(tag: u8, r: &mut Reader<'_>) -> Result<Self> {
        Ok(match tag {
            12 => Self::Provision(r.facts()?),
            13 => Self::Observe(read_identity(r)?),
            14 => Self::Clear(read_draft(r)?),
            15 => {
                let identity = read_identity(r)?;
                let count = r.u8()?;
                if !(1..=4).contains(&count) {
                    return Err(ProtocolError::Malformed);
                }
                let keys = (0..count)
                    .map(|_| Ok(r.u32()? as i32))
                    .collect::<Result<Vec<_>>>()?;
                if !valid_keys(&keys) {
                    return Err(ProtocolError::Malformed);
                }
                Self::AwaitKey { identity, keys }
            }
            _ => return Err(ProtocolError::Malformed),
        })
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Reply {
    Provisioned(Outcome<RangeState>),
    Observed(Outcome<Draft>),
    Cleared(Outcome<ClearReceipt>),
    KeyObserved(Outcome<KeyObservation>),
}

impl Reply {
    pub fn is_unknown(&self) -> bool {
        matches!(
            self,
            Self::Provisioned(Outcome::Unknown { .. })
                | Self::Observed(Outcome::Unknown { .. })
                | Self::Cleared(Outcome::Unknown { .. })
                | Self::KeyObserved(Outcome::Unknown { .. })
        )
    }

    pub(super) fn encode(&self, w: &mut Writer) -> Result<()> {
        match self {
            Self::Provisioned(outcome) => {
                w.u8(14);
                outcome.encode(w, true, write_state)
            }
            Self::Observed(outcome) => {
                w.u8(15);
                outcome.encode(w, false, write_draft)
            }
            Self::Cleared(outcome) => {
                w.u8(16);
                outcome.encode(w, true, |w, receipt| {
                    write_identity(w, &receipt.identity);
                    write_revision(w, receipt.before_revision)?;
                    write_revision(w, receipt.after_revision)
                })
            }
            Self::KeyObserved(outcome) => {
                w.u8(17);
                outcome.encode(w, false, |w, event| {
                    if event.input_serial == 0 || !valid_key(event.key.key) {
                        return Err(ProtocolError::Malformed);
                    }
                    write_draft(w, &event.draft)?;
                    w.uuid(event.event);
                    w.u64(event.input_serial);
                    w.u32(event.key.key as u32);
                    w.u16(event.key.count);
                    w.u32(event.key.modifiers);
                    w.u32(event.key.native_scan_code);
                    w.u32(event.key.native_virtual_key);
                    w.u32(event.key.native_modifiers);
                    w.u8(u8::from(event.key.auto_repeat));
                    w.u8(u8::from(event.key.spontaneous));
                    write_units(w, &event.key.text, MAX_KEY_TEXT_UNITS)
                })
            }
        }
    }

    pub(super) fn decode(tag: u8, r: &mut Reader<'_>) -> Result<Self> {
        Ok(match tag {
            14 => Self::Provisioned(Outcome::decode(r, true, read_state)?),
            15 => Self::Observed(Outcome::decode(r, false, read_draft)?),
            16 => Self::Cleared(Outcome::decode(r, true, |r| {
                Ok(ClearReceipt {
                    identity: read_identity(r)?,
                    before_revision: read_revision(r)?,
                    after_revision: read_revision(r)?,
                })
            })?),
            17 => Self::KeyObserved(Outcome::decode(r, false, |r| {
                let draft = read_draft(r)?;
                let event = r.uuid()?;
                let input_serial = r.u64()?;
                let key = r.u32()? as i32;
                if input_serial == 0 || !valid_key(key) {
                    return Err(ProtocolError::Malformed);
                }
                Ok(KeyObservation {
                    draft,
                    event,
                    input_serial,
                    key: KeyFacts {
                        key,
                        count: r.u16()?,
                        modifiers: r.u32()?,
                        native_scan_code: r.u32()?,
                        native_virtual_key: r.u32()?,
                        native_modifiers: r.u32()?,
                        auto_repeat: read_bool(r)?,
                        spontaneous: read_bool(r)?,
                        text: read_units(r, MAX_KEY_TEXT_UNITS)?,
                    },
                })
            })?),
            _ => return Err(ProtocolError::Malformed),
        })
    }

    pub(super) fn corresponds(&self, command: &Command) -> bool {
        // Session also accepts directly constructed frames. Enforce the same
        // bounds/status rules as the decoder before consuming its pending request.
        if self.encode(&mut Writer(Vec::new())).is_err() {
            return false;
        }
        match (command, self) {
            (Command::Provision(expected), Self::Provisioned(outcome)) => {
                outcome.agrees(|value| value.identity.facts == *expected)
            }
            (Command::Observe(expected), Self::Observed(outcome)) => {
                outcome.agrees(|value| value.state.identity == *expected)
            }
            (Command::Clear(expected), Self::Cleared(outcome)) => outcome.agrees(|value| {
                value.identity == expected.state.identity
                    && value.before_revision == expected.state.revision
            }),
            (Command::AwaitKey { identity, keys }, Self::KeyObserved(outcome)) => {
                outcome.agrees(|value| {
                    value.draft.state.identity == *identity && keys.contains(&value.key.key)
                })
            }
            _ => false,
        }
    }
}

fn write_identity(w: &mut Writer, identity: &RangeIdentity) {
    w.facts(&identity.facts);
    w.uuid(identity.visit);
    w.uuid(identity.range);
}

fn read_identity(r: &mut Reader<'_>) -> Result<RangeIdentity> {
    Ok(RangeIdentity {
        facts: r.facts()?,
        visit: r.uuid()?,
        range: r.uuid()?,
    })
}

fn write_revision(w: &mut Writer, revision: i32) -> Result<()> {
    if revision < 0 {
        return Err(ProtocolError::Malformed);
    }
    w.u32(revision as u32);
    Ok(())
}

fn read_revision(r: &mut Reader<'_>) -> Result<i32> {
    let revision = r.u32()? as i32;
    if revision < 0 {
        return Err(ProtocolError::Malformed);
    }
    Ok(revision)
}

fn write_state(w: &mut Writer, state: &RangeState) -> Result<()> {
    write_identity(w, &state.identity);
    write_revision(w, state.revision)
}

fn read_state(r: &mut Reader<'_>) -> Result<RangeState> {
    Ok(RangeState {
        identity: read_identity(r)?,
        revision: read_revision(r)?,
    })
}

fn write_units(w: &mut Writer, units: &[u16], maximum: usize) -> Result<()> {
    if units.len() > maximum {
        return Err(ProtocolError::TooLarge);
    }
    w.u16(units.len() as u16);
    for unit in units {
        w.u16(*unit);
    }
    Ok(())
}

fn read_units(r: &mut Reader<'_>, maximum: usize) -> Result<Vec<u16>> {
    let count = usize::from(r.u16()?);
    if count > maximum {
        return Err(ProtocolError::TooLarge);
    }
    if count > r.0.len() / 2 {
        return Err(ProtocolError::Malformed);
    }
    (0..count).map(|_| r.u16()).collect()
}

fn write_draft(w: &mut Writer, draft: &Draft) -> Result<()> {
    write_state(w, &draft.state)?;
    write_units(w, &draft.units, MAX_DRAFT_UNITS)
}

fn read_draft(r: &mut Reader<'_>) -> Result<Draft> {
    Ok(Draft {
        state: read_state(r)?,
        units: read_units(r, MAX_DRAFT_UNITS)?,
    })
}

fn read_bool(r: &mut Reader<'_>) -> Result<bool> {
    match r.u8()? {
        0 => Ok(false),
        1 => Ok(true),
        _ => Err(ProtocolError::Malformed),
    }
}
