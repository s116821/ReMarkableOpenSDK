//! Resident protocol primitives. No runtime activation or qualified native Platform.
//!
//! Decoded facts are peer assertions, not operational leases. The endpoint must
//! independently check peer credentials and the approved runtime profile.
use crate::{OperationId, PageKey, Uuid};
use sha2::{Digest, Sha256};
use std::collections::HashSet;

#[cfg(target_os = "linux")]
pub mod transport;

pub const PROTOCOL_REVISION: u16 = 1;
pub const MAX_FRAME: usize = 65_536;
pub const MAX_PAGES: usize = 2_048;
const MAGIC: &[u8; 4] = b"RSDK";

/// One-use launch admission for an independently verified native artifact.
/// Encoding does not create a file, launch a process, or grant native capability.
/// The supervisor must create an owned mode0700 activation directory and a
/// new mode0600 admission.bin, and must remain the adapter process's direct
/// parent and authenticated socket peer. Never erase admission.consumed to retry.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ActivationAdmission {
    pub profile_sha256: [u8; 32],
    pub executable_sha256: [u8; 32],
    pub adapter_sha256: [u8; 32],
    pub boot: Uuid,
    pub activation: Uuid,
    pub supervisor_uid: u32,
    pub supervisor_pid: u32,
    pub supervisor_start_ticks: u64,
    /// Absolute Linux CLOCK_BOOTTIME milliseconds, not wall-clock time.
    pub expires_boot_ms: u64,
    pub operation_budget_ms: u32,
}
impl ActivationAdmission {
    /// Canonical 168-byte bootstrap record. The endpoint independently checks
    /// the same bound at consumption; this caller-supplied clock is not proof.
    pub fn encode(&self, now_boot_ms: u64) -> Result<[u8; 168]> {
        if self.profile_sha256 == [0; 32]
            || self.executable_sha256 == [0; 32]
            || self.adapter_sha256 == [0; 32]
            || self.supervisor_pid <= 1
            || self.supervisor_start_ticks == 0
            || now_boot_ms == 0
            || self.expires_boot_ms <= now_boot_ms
            || self.expires_boot_ms - now_boot_ms > 60_000
            || !(1..=30_000).contains(&self.operation_budget_ms)
        {
            return Err(ProtocolError::InvalidIdentity);
        }
        let mut bytes = [0; 168];
        bytes[..8].copy_from_slice(b"RSDKACT1");
        bytes[8..40].copy_from_slice(&self.profile_sha256);
        bytes[40..72].copy_from_slice(&self.executable_sha256);
        bytes[72..104].copy_from_slice(&self.adapter_sha256);
        bytes[104..120].copy_from_slice(self.boot.as_bytes());
        bytes[120..136].copy_from_slice(self.activation.as_bytes());
        bytes[136..140].copy_from_slice(&self.supervisor_uid.to_be_bytes());
        bytes[140..144].copy_from_slice(&self.supervisor_pid.to_be_bytes());
        bytes[144..152].copy_from_slice(&self.supervisor_start_ticks.to_be_bytes());
        bytes[152..160].copy_from_slice(&self.expires_boot_ms.to_be_bytes());
        bytes[160..164].copy_from_slice(&self.operation_budget_ms.to_be_bytes());
        Ok(bytes)
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum ProtocolError {
    Malformed,
    TooLarge,
    UnsupportedVersion,
    InvalidIdentity,
    InvalidOrder,
    WrongScope,
    WrongResponse,
    Busy,
    Closed,
    SequenceExhausted,
}
type Result<T> = std::result::Result<T, ProtocolError>;

/// Supplied by the supervisor after checking the actual runtime and payload.
/// Protocol equality alone does not establish these assertions.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct RuntimeIdentity {
    pub profile_sha256: [u8; 32],
    pub executable_sha256: [u8; 32],
    pub adapter_sha256: [u8; 32],
    pub boot: Uuid,
    pub pid: u32,
    pub start_ticks: u64,
    pub activation: Uuid,
}
impl RuntimeIdentity {
    fn validate(&self) -> Result<()> {
        if self.pid == 0
            || self.start_ticks == 0
            || self.profile_sha256 == [0; 32]
            || self.executable_sha256 == [0; 32]
            || self.adapter_sha256 == [0; 32]
        {
            return Err(ProtocolError::InvalidIdentity);
        }
        Ok(())
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ConnectionIdentity {
    pub runtime: RuntimeIdentity,
    /// Fresh caller OS nonce; never reuse after reconnect, including same PID.
    pub connection: Uuid,
    /// Absent only in Hello; generated freshly by the endpoint for Ready.
    pub server_nonce: Option<Uuid>,
}

/// Historical actual-Reader identity and observed order; no content revision,
/// input epoch, persistence or current ownership is implied.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct ReaderFacts {
    page: PageKey,
    order: Vec<Uuid>,
}
impl ReaderFacts {
    pub fn new(page: PageKey, order: Vec<Uuid>) -> Result<Self> {
        validate_order(&order)?;
        if !order.contains(&page.page) {
            return Err(ProtocolError::InvalidOrder);
        }
        Ok(Self { page, order })
    }
    pub fn page(&self) -> PageKey {
        self.page
    }
    pub fn order(&self) -> &[Uuid] {
        &self.order
    }
}

/// Immutable expected relation. The native executor must check this again in
/// the original insertion critical section, including target absence from the
/// full native tree (not only active order). Construction grants no admission.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct InsertAfter {
    pub source: ReaderFacts,
    pub target: Uuid,
}
impl InsertAfter {
    fn validate(&self) -> Result<()> {
        if self.source.order.contains(&self.target) {
            return Err(ProtocolError::InvalidOrder);
        }
        Ok(())
    }
    pub fn expected_after_order(&self) -> Vec<Uuid> {
        let mut order = self.source.order.clone();
        // ReaderFacts construction/decoding proves exactly one source in order.
        let position = order
            .iter()
            .position(|id| *id == self.source.page.page)
            .unwrap();
        order.insert(position + 1, self.target);
        order
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Command {
    Observe,
    InsertAfter(InsertAfter),
    /// Read-only; target presence cannot by itself establish attribution.
    Reconcile(InsertAfter),
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Rejection {
    Unsupported,
    BeforeDispatch,
    ExecutorBeforeMutation,
    UnknownIdentity,
    Capacity,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Reply {
    Observed(ReaderFacts),
    /// Attributed native forward plus exact after-order, NOT durable commit.
    InsertionObserved {
        target: PageKey,
        after_order: Vec<Uuid>,
    },
    Reconciled {
        current: ReaderFacts,
        target_present: bool,
    },
    Rejected(Rejection),
    Indeterminate,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Message {
    Hello,
    Ready,
    Request(Command),
    Response {
        request_sha256: [u8; 32],
        reply: Reply,
    },
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Frame {
    pub scope: ConnectionIdentity,
    pub sequence: u64,
    pub operation: OperationId,
    pub message: Message,
}

fn validate_order(order: &[Uuid]) -> Result<()> {
    if order.is_empty()
        || order.len() > MAX_PAGES
        || order.iter().collect::<HashSet<_>>().len() != order.len()
    {
        return Err(ProtocolError::InvalidOrder);
    }
    Ok(())
}

impl Frame {
    /// Canonical encoding, including the four-byte length prefix.
    pub fn encode(&self) -> Result<Vec<u8>> {
        self.scope.runtime.validate()?;
        if matches!(self.message, Message::Hello) != self.scope.server_nonce.is_none() {
            return Err(ProtocolError::InvalidIdentity);
        }
        if matches!(self.message, Message::Hello | Message::Ready) != (self.sequence == 0) {
            return Err(ProtocolError::Malformed);
        }
        let mut w = Writer(Vec::new());
        w.0.extend(MAGIC);
        w.u16(PROTOCOL_REVISION);
        let r = &self.scope.runtime;
        w.0.extend(r.profile_sha256);
        w.0.extend(r.executable_sha256);
        w.0.extend(r.adapter_sha256);
        w.uuid(r.boot);
        w.u32(r.pid);
        w.u64(r.start_ticks);
        w.uuid(r.activation);
        w.uuid(self.scope.connection);
        w.0.extend(self.scope.server_nonce.map_or([0; 16], |id| *id.as_bytes()));
        w.u64(self.sequence);
        w.uuid(self.operation.0);
        match &self.message {
            Message::Hello => w.u8(0),
            Message::Ready => w.u8(1),
            Message::Request(command) => {
                w.u8(2);
                match command {
                    Command::Observe => w.u8(0),
                    Command::InsertAfter(value) | Command::Reconcile(value) => {
                        value.validate()?;
                        if matches!(command, Command::InsertAfter(_))
                            && value.source.order.len() == MAX_PAGES
                        {
                            return Err(ProtocolError::InvalidOrder);
                        }
                        w.u8(if matches!(command, Command::InsertAfter(_)) {
                            1
                        } else {
                            2
                        });
                        w.facts(&value.source);
                        w.uuid(value.target);
                    }
                }
            }
            Message::Response {
                request_sha256,
                reply,
            } => {
                w.u8(3);
                w.0.extend(request_sha256);
                match reply {
                    Reply::Observed(facts) => {
                        w.u8(0);
                        w.facts(facts);
                    }
                    Reply::InsertionObserved {
                        target,
                        after_order,
                    } => {
                        validate_order(after_order)?;
                        if !after_order.contains(&target.page) {
                            return Err(ProtocolError::InvalidOrder);
                        }
                        w.u8(1);
                        w.page(*target);
                        w.order(after_order);
                    }
                    Reply::Reconciled {
                        current,
                        target_present,
                    } => {
                        w.u8(2);
                        w.facts(current);
                        w.u8(u8::from(*target_present));
                    }
                    Reply::Rejected(reason) => {
                        w.u8(3);
                        w.u8(match reason {
                            Rejection::Unsupported => 0,
                            Rejection::BeforeDispatch => 1,
                            Rejection::ExecutorBeforeMutation => 2,
                            Rejection::UnknownIdentity => 3,
                            Rejection::Capacity => 4,
                        });
                    }
                    Reply::Indeterminate => w.u8(4),
                }
            }
        }
        if w.0.len() > MAX_FRAME {
            return Err(ProtocolError::TooLarge);
        }
        let mut bytes = (w.0.len() as u32).to_be_bytes().to_vec();
        bytes.extend(w.0);
        Ok(bytes)
    }

    /// Decode one payload, without the length prefix. Reject trailing bytes.
    pub fn decode(payload: &[u8]) -> Result<Self> {
        if payload.len() > MAX_FRAME {
            return Err(ProtocolError::TooLarge);
        }
        let mut r = Reader(payload);
        if &r.take::<4>()? != MAGIC {
            return Err(ProtocolError::Malformed);
        }
        if r.u16()? != PROTOCOL_REVISION {
            return Err(ProtocolError::UnsupportedVersion);
        }
        let runtime = RuntimeIdentity {
            profile_sha256: r.take()?,
            executable_sha256: r.take()?,
            adapter_sha256: r.take()?,
            boot: r.uuid()?,
            pid: r.u32()?,
            start_ticks: r.u64()?,
            activation: r.uuid()?,
        };
        runtime.validate()?;
        let scope = ConnectionIdentity {
            runtime,
            connection: r.uuid()?,
            server_nonce: {
                let bytes = r.take::<16>()?;
                (bytes != [0; 16]).then_some(Uuid(bytes))
            },
        };
        let sequence = r.u64()?;
        let operation = OperationId(r.uuid()?);
        let message = match r.u8()? {
            0 => Message::Hello,
            1 => Message::Ready,
            2 => Message::Request(match r.u8()? {
                0 => Command::Observe,
                tag @ (1 | 2) => {
                    let value = InsertAfter {
                        source: r.facts()?,
                        target: r.uuid()?,
                    };
                    value.validate()?;
                    if tag == 1 && value.source.order.len() == MAX_PAGES {
                        return Err(ProtocolError::InvalidOrder);
                    }
                    if tag == 1 {
                        Command::InsertAfter(value)
                    } else {
                        Command::Reconcile(value)
                    }
                }
                _ => return Err(ProtocolError::Malformed),
            }),
            3 => {
                let request_sha256 = r.take()?;
                let reply = match r.u8()? {
                    0 => Reply::Observed(r.facts()?),
                    1 => {
                        let target = r.page()?;
                        let after_order = r.order()?;
                        if !after_order.contains(&target.page) {
                            return Err(ProtocolError::InvalidOrder);
                        }
                        Reply::InsertionObserved {
                            target,
                            after_order,
                        }
                    }
                    2 => Reply::Reconciled {
                        current: r.facts()?,
                        target_present: match r.u8()? {
                            0 => false,
                            1 => true,
                            _ => return Err(ProtocolError::Malformed),
                        },
                    },
                    3 => Reply::Rejected(match r.u8()? {
                        0 => Rejection::Unsupported,
                        1 => Rejection::BeforeDispatch,
                        2 => Rejection::ExecutorBeforeMutation,
                        3 => Rejection::UnknownIdentity,
                        4 => Rejection::Capacity,
                        _ => return Err(ProtocolError::Malformed),
                    }),
                    4 => Reply::Indeterminate,
                    _ => return Err(ProtocolError::Malformed),
                };
                Message::Response {
                    request_sha256,
                    reply,
                }
            }
            _ => return Err(ProtocolError::Malformed),
        };
        if !r.0.is_empty() || matches!(message, Message::Hello | Message::Ready) != (sequence == 0)
        {
            return Err(ProtocolError::Malformed);
        }
        if matches!(message, Message::Hello) != scope.server_nonce.is_none() {
            return Err(ProtocolError::InvalidIdentity);
        }
        Ok(Self {
            scope,
            sequence,
            operation,
            message,
        })
    }

    pub fn fingerprint(&self) -> Result<[u8; 32]> {
        Ok(Sha256::digest(self.encode()?).into())
    }
}

struct Writer(Vec<u8>);
impl Writer {
    fn u8(&mut self, v: u8) {
        self.0.push(v);
    }
    fn u16(&mut self, v: u16) {
        self.0.extend(v.to_be_bytes());
    }
    fn u32(&mut self, v: u32) {
        self.0.extend(v.to_be_bytes());
    }
    fn u64(&mut self, v: u64) {
        self.0.extend(v.to_be_bytes());
    }
    fn uuid(&mut self, v: Uuid) {
        self.0.extend(v.as_bytes());
    }
    fn page(&mut self, v: PageKey) {
        self.uuid(v.document);
        self.uuid(v.page);
    }
    fn order(&mut self, v: &[Uuid]) {
        self.u16(v.len() as u16);
        for id in v {
            self.uuid(*id);
        }
    }
    fn facts(&mut self, v: &ReaderFacts) {
        self.page(v.page);
        self.order(&v.order);
    }
}
struct Reader<'a>(&'a [u8]);
impl Reader<'_> {
    fn take<const N: usize>(&mut self) -> Result<[u8; N]> {
        if self.0.len() < N {
            return Err(ProtocolError::Malformed);
        }
        let (value, remaining) = self.0.split_at(N);
        self.0 = remaining;
        Ok(value.try_into().unwrap())
    }
    fn u8(&mut self) -> Result<u8> {
        Ok(self.take::<1>()?[0])
    }
    fn u16(&mut self) -> Result<u16> {
        Ok(u16::from_be_bytes(self.take()?))
    }
    fn u32(&mut self) -> Result<u32> {
        Ok(u32::from_be_bytes(self.take()?))
    }
    fn u64(&mut self) -> Result<u64> {
        Ok(u64::from_be_bytes(self.take()?))
    }
    fn uuid(&mut self) -> Result<Uuid> {
        let bytes = self.take()?;
        if bytes == [0; 16] {
            return Err(ProtocolError::InvalidIdentity);
        }
        Ok(Uuid(bytes))
    }
    fn page(&mut self) -> Result<PageKey> {
        Ok(PageKey {
            document: self.uuid()?,
            page: self.uuid()?,
        })
    }
    fn order(&mut self) -> Result<Vec<Uuid>> {
        let count = usize::from(self.u16()?);
        if count == 0 || count > MAX_PAGES || count > self.0.len() / 16 {
            return Err(ProtocolError::InvalidOrder);
        }
        let order: Vec<_> = (0..count).map(|_| self.uuid()).collect::<Result<_>>()?;
        validate_order(&order)?;
        Ok(order)
    }
    fn facts(&mut self) -> Result<ReaderFacts> {
        ReaderFacts::new(self.page()?, self.order()?)
    }
}

/// Incremental decoder with a fixed memory bound even if fed an enormous slice.
/// A malformed stream is permanently poisoned; it cannot resynchronize/replay.
#[derive(Default)]
pub struct Decoder {
    bytes: Vec<u8>,
    poisoned: bool,
}
impl Decoder {
    /// Consume at most one frame. Caller retains the remainder of the input.
    pub fn feed(&mut self, input: &[u8]) -> Result<(usize, Option<Frame>)> {
        if self.poisoned {
            return Err(ProtocolError::Closed);
        }
        let result = self.feed_inner(input);
        if result.is_err() {
            self.poisoned = true;
            self.bytes.clear();
        }
        result
    }
    fn feed_inner(&mut self, input: &[u8]) -> Result<(usize, Option<Frame>)> {
        let mut used = 0;
        if self.bytes.len() < 4 {
            let n = (4 - self.bytes.len()).min(input.len());
            self.bytes.extend_from_slice(&input[..n]);
            used += n;
            if self.bytes.len() < 4 {
                return Ok((used, None));
            }
        }
        let size = u32::from_be_bytes(self.bytes[..4].try_into().unwrap()) as usize;
        if size > MAX_FRAME {
            return Err(ProtocolError::TooLarge);
        }
        if size == 0 {
            return Err(ProtocolError::Malformed);
        }
        let n = (4 + size - self.bytes.len()).min(input.len() - used);
        self.bytes.extend_from_slice(&input[used..used + n]);
        used += n;
        if self.bytes.len() != 4 + size {
            return Ok((used, None));
        }
        let frame = Frame::decode(&self.bytes[4..])?;
        self.bytes.clear();
        Ok((used, Some(frame)))
    }
    /// EOF never produces a frame; it invalidates this decoder permanently.
    pub fn close(&mut self) {
        self.poisoned = true;
        self.bytes.clear();
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum LostRequest {
    /// No unanswered mutation on this connection. Earlier observed mutations
    /// and their persistence/binding journal obligations are not undone.
    NoPendingMutation,
    Indeterminate {
        operation: OperationId,
        request_sha256: [u8; 32],
    },
}

/// Client-side correspondence. There is deliberately no automatic reconnect or
/// retry. Only one pending operation is retained, independent of session length.
pub struct Session {
    scope: ConnectionIdentity,
    handshake_operation: OperationId,
    ready: bool,
    closed: bool,
    sequence: u64,
    pending: Option<(Frame, [u8; 32])>,
    terminal_loss: LostRequest,
}
impl Session {
    pub fn new(scope: ConnectionIdentity, handshake_operation: OperationId) -> Result<Self> {
        scope.runtime.validate()?;
        if scope.server_nonce.is_some() {
            return Err(ProtocolError::InvalidIdentity);
        }
        Ok(Self {
            scope,
            handshake_operation,
            ready: false,
            closed: false,
            sequence: 0,
            pending: None,
            terminal_loss: LostRequest::NoPendingMutation,
        })
    }
    pub fn hello(&self) -> Result<Frame> {
        if self.closed || self.ready {
            return Err(ProtocolError::Closed);
        }
        Ok(Frame {
            scope: self.scope.clone(),
            sequence: 0,
            operation: self.handshake_operation,
            message: Message::Hello,
        })
    }
    pub fn accept_ready(&mut self, frame: &Frame) -> Result<()> {
        if self.closed || self.ready {
            return Err(ProtocolError::Closed);
        }
        let mut expected = self.scope.clone();
        expected.server_nonce = frame.scope.server_nonce;
        if frame.scope != expected {
            return Err(ProtocolError::WrongScope);
        }
        if frame.sequence != 0
            || frame.operation != self.handshake_operation
            || frame.message != Message::Ready
        {
            return Err(ProtocolError::WrongResponse);
        }
        if frame.scope.server_nonce.is_none() {
            return Err(ProtocolError::InvalidIdentity);
        }
        self.scope = expected;
        self.ready = true;
        Ok(())
    }
    /// Call immediately before the first transport write, AFTER journal persist.
    /// Once returned, even zero/partial write failure is conservatively unknown
    /// for mutation. The caller must not invoke this again to retry that request.
    pub fn begin(&mut self, operation: OperationId, command: Command) -> Result<Frame> {
        let frame = match self.prepare(operation, command) {
            Err(ProtocolError::SequenceExhausted) => {
                self.closed = true;
                return Err(ProtocolError::SequenceExhausted);
            }
            result => result?,
        };
        let digest = frame.fingerprint()?;
        self.sequence = frame.sequence;
        self.pending = Some((frame.clone(), digest));
        Ok(frame)
    }
    /// Preview the exact next request for durable journaling. No sequence is
    /// consumed and no operation becomes pending until `begin` is called.
    pub fn prepare(&self, operation: OperationId, command: Command) -> Result<Frame> {
        if self.closed || !self.ready {
            return Err(ProtocolError::Closed);
        }
        if self.pending.is_some() {
            return Err(ProtocolError::Busy);
        }
        let Some(sequence) = self.sequence.checked_add(1) else {
            return Err(ProtocolError::SequenceExhausted);
        };
        let frame = Frame {
            scope: self.scope.clone(),
            sequence,
            operation,
            message: Message::Request(command),
        };
        frame.encode()?;
        Ok(frame)
    }
    /// A mismatch retains pending ambiguity; it cannot consume the operation.
    pub fn complete(&mut self, frame: &Frame) -> Result<Reply> {
        if self.closed || !self.ready {
            return Err(ProtocolError::Closed);
        }
        if frame.scope != self.scope {
            return Err(ProtocolError::WrongScope);
        }
        let Some((request, digest)) = &self.pending else {
            return Err(ProtocolError::WrongResponse);
        };
        let Message::Response {
            request_sha256,
            reply,
        } = &frame.message
        else {
            return Err(ProtocolError::WrongResponse);
        };
        if frame.sequence != request.sequence
            || frame.operation != request.operation
            || request_sha256 != digest
        {
            return Err(ProtocolError::WrongResponse);
        }
        let Message::Request(command) = &request.message else {
            unreachable!()
        };
        let valid = match (command, reply) {
            (Command::Observe, Reply::Observed(_)) => true,
            (
                Command::InsertAfter(value),
                Reply::InsertionObserved {
                    target,
                    after_order,
                },
            ) => {
                *target
                    == PageKey {
                        document: value.source.page.document,
                        page: value.target,
                    }
                    && *after_order == value.expected_after_order()
            }
            (
                Command::Reconcile(value),
                Reply::Reconciled {
                    current,
                    target_present,
                },
            ) => {
                current.page.document == value.source.page.document
                    && *target_present == current.order.contains(&value.target)
            }
            (_, Reply::Rejected(_) | Reply::Indeterminate) => true,
            _ => false,
        };
        if !valid {
            return Err(ProtocolError::WrongResponse);
        }
        if matches!(command, Command::InsertAfter(_)) && *reply == Reply::Indeterminate {
            let reply = reply.clone();
            self.close();
            return Ok(reply);
        }
        self.pending = None;
        Ok(reply.clone())
    }
    /// Local logical retirement. This does not cancel a native worker or reclaim
    /// its owners. Close the transport; late replies cannot restore this session.
    pub fn close(&mut self) -> LostRequest {
        if self.closed {
            return self.terminal_loss.clone();
        }
        self.closed = true;
        self.terminal_loss = match self.pending.take() {
            Some((
                Frame {
                    operation,
                    message: Message::Request(Command::InsertAfter(_)),
                    ..
                },
                request_sha256,
            )) => LostRequest::Indeterminate {
                operation,
                request_sha256,
            },
            _ => LostRequest::NoPendingMutation,
        };
        self.terminal_loss.clone()
    }
}

#[cfg(test)]
mod session_limits {
    use super::*;

    #[test]
    fn exhausted_sequence_closes_instead_of_wrapping() {
        let uuid = Uuid([1; 16]);
        let runtime = RuntimeIdentity {
            profile_sha256: [1; 32],
            executable_sha256: [2; 32],
            adapter_sha256: [3; 32],
            boot: uuid,
            pid: 1,
            start_ticks: 1,
            activation: uuid,
        };
        let mut session = Session::new(
            ConnectionIdentity {
                runtime,
                connection: uuid,
                server_nonce: None,
            },
            OperationId(uuid),
        )
        .unwrap();
        let mut ready = session.hello().unwrap();
        ready.message = Message::Ready;
        ready.scope.server_nonce = Some(Uuid([2; 16]));
        session.accept_ready(&ready).unwrap();
        session.sequence = u64::MAX;
        assert_eq!(
            session.begin(OperationId(uuid), Command::Observe),
            Err(ProtocolError::SequenceExhausted)
        );
        assert_eq!(
            session.begin(OperationId(uuid), Command::Observe),
            Err(ProtocolError::Closed)
        );
        assert_eq!(session.close(), LostRequest::NoPendingMutation);
    }
}
