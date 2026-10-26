//! Linux nonblocking transport for a supervisor-verified resident runtime.
//!
//! Kernel credentials authenticate a process incarnation, not its code. The
//! supervisor must independently verify artifact hashes, retain the owned 0700
//! activation directory descriptor, generate fresh nonces, and own recovery.
//! This module neither launches a runtime nor grants native capabilities.
use super::*;
use nix::errno::Errno;
use nix::poll::{PollFd, PollFlags, PollTimeout, poll};
use nix::sys::socket::{
    AddressFamily, SockFlag, SockType, UnixAddr, connect, getsockopt, socket, sockopt,
};
use std::fs::{self, File, Metadata};
use std::io::{self, Read, Write};
use std::net::Shutdown;
use std::os::fd::{AsFd, AsRawFd};
use std::os::unix::fs::{FileTypeExt, MetadataExt};
use std::os::unix::net::UnixStream;
use std::path::PathBuf;
use std::time::Instant;

#[derive(Debug)]
pub enum Failure {
    Io(io::Error),
    Protocol(ProtocolError),
    Identity,
    Deadline,
    Journal(io::Error),
    Progress(io::Error),
}
impl From<io::Error> for Failure {
    fn from(error: io::Error) -> Self {
        Self::Io(error)
    }
}
impl From<Errno> for Failure {
    fn from(error: Errno) -> Self {
        Self::Io(io::Error::from_raw_os_error(error as i32))
    }
}
impl From<ProtocolError> for Failure {
    fn from(error: ProtocolError) -> Self {
        Self::Protocol(error)
    }
}

/// Every request failure carries sticky original mutation ambiguity, if any.
#[derive(Debug)]
pub struct TransportError {
    pub cause: Failure,
    pub lost: LostRequest,
}
type TransportResult<T> = std::result::Result<T, Failure>;

/// A single authenticated connection. No reconnect, replay, socket cloning or
/// raw write escape hatch is exposed. Dropping closes only this connection.
pub struct Client {
    stream: UnixStream,
    directory: File,
    socket_identity: (u64, u64),
    uid: u32,
    runtime: RuntimeIdentity,
    session: Session,
}
impl Client {
    /// Consumes the supervisor's pinned directory descriptor. `scope` must come
    /// from independent activation verification, never a peer's Hello/Ready.
    /// The deadline covers connect and the entire handshake, including fragments.
    pub fn connect(
        directory: File,
        uid: u32,
        scope: ConnectionIdentity,
        handshake: OperationId,
        deadline: Instant,
    ) -> TransportResult<Self> {
        Self::connect_with_progress(directory, uid, scope, handshake, deadline, &mut || Ok(()))
    }

    /// Like `connect`, with caller-thread cancellation/health servicing during
    /// IO. The callback must be bounded; its execution never renews the deadline.
    /// Failure closes this connection and does not prove native cancellation.
    pub fn connect_with_progress(
        directory: File,
        uid: u32,
        scope: ConnectionIdentity,
        handshake: OperationId,
        deadline: Instant,
        progress: &mut dyn FnMut() -> io::Result<()>,
    ) -> TransportResult<Self> {
        let mut progress = || progress().map_err(Failure::Progress);
        check_progress(deadline, &mut progress)?;
        let session = Session::new(scope.clone(), handshake)?;
        check_directory(&directory, uid)?;
        let path = socket_path(&directory);
        let metadata = fs::symlink_metadata(&path)?;
        check_socket(&metadata, uid)?;
        let socket_identity = (metadata.dev(), metadata.ino());
        let fd = socket(
            AddressFamily::Unix,
            SockType::Stream,
            SockFlag::SOCK_CLOEXEC | SockFlag::SOCK_NONBLOCK,
            None,
        )?;
        let address = UnixAddr::new(&path)?;
        check_progress(deadline, &mut progress)?;
        let pending = match connect(fd.as_raw_fd(), &address) {
            Ok(()) => false,
            Err(Errno::EINPROGRESS) => true,
            // A full Unix listener backlog (EAGAIN) is a refusal, never retried.
            Err(error) => return Err(error.into()),
        };
        let stream = UnixStream::from(fd);
        if pending {
            wait(&stream, PollFlags::POLLOUT, deadline, &mut progress)?;
            if let Some(error) = stream.take_error()? {
                return Err(error.into());
            }
        }
        let mut client = Self {
            stream,
            directory,
            socket_identity,
            uid,
            runtime: scope.runtime,
            session,
        };
        let Self {
            stream,
            directory,
            socket_identity,
            uid,
            runtime,
            session,
        } = &mut client;
        let mut serviced = || {
            check_progress(deadline, &mut progress)?;
            authenticate(stream, directory, *socket_identity, *uid, runtime)
        };
        check_progress(deadline, &mut serviced)?;
        let hello = session.hello()?.encode()?;
        write_all(stream, &hello, deadline, &mut serviced)?;
        let ready = read_frame(stream, deadline, &mut serviced)?;
        check_progress(deadline, &mut serviced)?;
        session.accept_ready(&ready)?;
        Ok(client)
    }

    /// The journal callback must durably persist the exact request and digest
    /// before returning success for mutations. It runs before any request write.
    /// It must also enforce operation deduplication across connections. A
    /// callback is not proof of either guarantee; the consumer owns the journal,
    /// fsync and recovery. The callback itself is synchronous and cannot be
    /// forcibly bounded here; expiry after it returns prevents request dispatch.
    /// No partial IO or interrupted syscall extends the absolute deadline.
    pub fn execute(
        &mut self,
        operation: OperationId,
        command: Command,
        deadline: Instant,
        journal: impl FnOnce(&Frame, [u8; 32]) -> io::Result<()>,
    ) -> std::result::Result<Reply, TransportError> {
        self.execute_with_progress(operation, command, deadline, journal, &mut || Ok(()))
    }

    /// Like `execute`, with progress checks before dispatch, during bounded IO
    /// waits and before accepting the reply. The callback runs synchronously on
    /// this thread and must be bounded and non-reentrant. Callback failure closes
    /// the connection; an admitted effect remains indeterminate even if complete
    /// response bytes arrived. Socket closure is not native owner reclamation.
    pub fn execute_with_progress(
        &mut self,
        operation: OperationId,
        command: Command,
        deadline: Instant,
        journal: impl FnOnce(&Frame, [u8; 32]) -> io::Result<()>,
        progress: &mut dyn FnMut() -> io::Result<()>,
    ) -> std::result::Result<Reply, TransportError> {
        let result = (|| {
            let Self {
                stream,
                directory,
                socket_identity,
                uid,
                runtime,
                session,
            } = self;
            let mut serviced = || {
                check_deadline(deadline)?;
                progress().map_err(Failure::Progress)?;
                check_deadline(deadline)?;
                authenticate(stream, directory, *socket_identity, *uid, runtime)
            };
            check_progress(deadline, &mut serviced)?;
            let prepared = session.prepare(operation, command.clone())?;
            let bytes = prepared.encode()?;
            let digest = prepared.fingerprint()?;
            journal(&prepared, digest).map_err(Failure::Journal)?;
            // Both callbacks may be slow or replace filesystem entries. Recheck
            // identity after progress at every IO boundary and final admission.
            check_progress(deadline, &mut serviced)?;
            session.begin(operation, command)?;
            write_all(stream, &bytes, deadline, &mut serviced)?;
            let response = read_frame(stream, deadline, &mut serviced)?;
            check_progress(deadline, &mut serviced)?;
            let reply = session.complete(&response)?;
            if reply == Reply::Indeterminate {
                self.close();
            }
            Ok(reply)
        })();
        result.map_err(|cause| TransportError {
            cause,
            lost: self.close(),
        })
    }

    /// Socket retirement does not cancel native work or reclaim native owners.
    pub fn close(&mut self) -> LostRequest {
        let lost = self.session.close();
        let _ = self.stream.shutdown(Shutdown::Both);
        lost
    }
}
fn authenticate(
    stream: &UnixStream,
    directory: &File,
    socket_identity: (u64, u64),
    uid: u32,
    runtime: &RuntimeIdentity,
) -> TransportResult<()> {
    check_directory(directory, uid)?;
    let metadata = fs::symlink_metadata(socket_path(directory))?;
    check_socket(&metadata, uid)?;
    if (metadata.dev(), metadata.ino()) != socket_identity {
        return Err(Failure::Identity);
    }
    let peer = getsockopt(stream, sockopt::PeerCredentials)?;
    if peer.pid() <= 0 || peer.pid() as u32 != runtime.pid || peer.uid() != uid {
        return Err(Failure::Identity);
    }
    let boot = bounded_text("/proc/sys/kernel/random/boot_id")?;
    let stat = bounded_text(&format!("/proc/{}/stat", runtime.pid))?;
    if Uuid::parse(boot.trim()) != Some(runtime.boot)
        || process_start(&stat) != Some(runtime.start_ticks)
    {
        return Err(Failure::Identity);
    }
    Ok(())
}
fn socket_path(directory: &File) -> PathBuf {
    PathBuf::from(format!("/proc/self/fd/{}/sdk.sock", directory.as_raw_fd()))
}
fn check_directory(directory: &File, uid: u32) -> TransportResult<()> {
    let metadata = directory.metadata()?;
    if !metadata.is_dir()
        || metadata.uid() != uid
        || metadata.mode() & 0o7777 != 0o700
        || metadata.nlink() == 0
    {
        return Err(Failure::Identity);
    }
    Ok(())
}
fn check_socket(metadata: &Metadata, uid: u32) -> TransportResult<()> {
    if !metadata.file_type().is_socket()
        || metadata.uid() != uid
        || metadata.mode() & 0o7777 != 0o600
        || metadata.nlink() != 1
    {
        return Err(Failure::Identity);
    }
    Ok(())
}
fn bounded_text(path: &str) -> TransportResult<String> {
    let mut text = String::new();
    File::open(path)?.take(4097).read_to_string(&mut text)?;
    if text.len() > 4096 {
        return Err(Failure::Identity);
    }
    Ok(text)
}
fn process_start(stat: &str) -> Option<u64> {
    // comm may contain spaces and ')'; fields after its final ')' start at 3.
    stat.rsplit_once(')')?
        .1
        .split_whitespace()
        .nth(19)?
        .parse()
        .ok()
}
fn check_deadline(deadline: Instant) -> TransportResult<()> {
    if Instant::now() >= deadline {
        Err(Failure::Deadline)
    } else {
        Ok(())
    }
}
fn check_progress(
    deadline: Instant,
    progress: &mut dyn FnMut() -> TransportResult<()>,
) -> TransportResult<()> {
    check_deadline(deadline)?;
    progress()?;
    check_deadline(deadline)
}

fn wait(
    stream: &UnixStream,
    flags: PollFlags,
    deadline: Instant,
    progress: &mut dyn FnMut() -> TransportResult<()>,
) -> TransportResult<()> {
    loop {
        check_progress(deadline, progress)?;
        let ms = deadline
            .saturating_duration_since(Instant::now())
            .as_millis()
            .saturating_add(1)
            .min(50);
        let mut fds = [PollFd::new(stream.as_fd(), flags)];
        match poll(
            &mut fds,
            PollTimeout::try_from(ms).expect("bounded timeout"),
        ) {
            Ok(0) | Err(Errno::EINTR) => continue,
            Ok(_) => {
                check_progress(deadline, progress)?;
                return Ok(());
            }
            Err(error) => return Err(error.into()),
        }
    }
}
fn write_all(
    mut stream: &UnixStream,
    mut bytes: &[u8],
    deadline: Instant,
    progress: &mut dyn FnMut() -> TransportResult<()>,
) -> TransportResult<()> {
    while !bytes.is_empty() {
        wait(stream, PollFlags::POLLOUT, deadline, progress)?;
        match stream.write(bytes) {
            Ok(0) => return Err(io::Error::from(io::ErrorKind::WriteZero).into()),
            Ok(n) => bytes = &bytes[n..],
            Err(e)
                if matches!(
                    e.kind(),
                    io::ErrorKind::WouldBlock | io::ErrorKind::Interrupted
                ) => {}
            Err(e) => return Err(e.into()),
        }
    }
    check_deadline(deadline)
}
fn read_exact(
    mut stream: &UnixStream,
    mut bytes: &mut [u8],
    deadline: Instant,
    progress: &mut dyn FnMut() -> TransportResult<()>,
) -> TransportResult<()> {
    while !bytes.is_empty() {
        wait(stream, PollFlags::POLLIN, deadline, progress)?;
        match stream.read(bytes) {
            Ok(0) => return Err(io::Error::from(io::ErrorKind::UnexpectedEof).into()),
            Ok(n) => bytes = &mut bytes[n..],
            Err(e)
                if matches!(
                    e.kind(),
                    io::ErrorKind::WouldBlock | io::ErrorKind::Interrupted
                ) => {}
            Err(e) => return Err(e.into()),
        }
    }
    check_deadline(deadline)
}
fn read_frame(
    stream: &UnixStream,
    deadline: Instant,
    progress: &mut dyn FnMut() -> TransportResult<()>,
) -> TransportResult<Frame> {
    let mut prefix = [0; 4];
    read_exact(stream, &mut prefix, deadline, progress)?;
    let size = u32::from_be_bytes(prefix) as usize;
    if size == 0 || size > MAX_FRAME {
        return Err(ProtocolError::TooLarge.into());
    }
    let mut bytes = vec![0; size];
    read_exact(stream, &mut bytes, deadline, progress)?;
    Ok(Frame::decode(&bytes)?)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::Duration;

    #[test]
    fn partial_write_expires_without_reset_or_full_delivery() {
        let (writer, mut reader) = UnixStream::pair().unwrap();
        writer.set_nonblocking(true).unwrap();
        reader.set_nonblocking(true).unwrap();
        nix::sys::socket::setsockopt(&writer, sockopt::SndBuf, &1024).unwrap();
        let bytes = vec![7; MAX_FRAME];
        assert!(matches!(
            write_all(
                &writer,
                &bytes,
                Instant::now() + Duration::from_millis(30),
                &mut || Ok(())
            ),
            Err(Failure::Deadline)
        ));
        let mut delivered = Vec::new();
        assert_eq!(
            reader.read_to_end(&mut delivered).unwrap_err().kind(),
            io::ErrorKind::WouldBlock
        );
        assert!(!delivered.is_empty() && delivered.len() < bytes.len());
        assert!(delivered.iter().all(|byte| *byte == 7));
    }

    #[test]
    fn proc_comm_parentheses_cannot_shift_start_field() {
        assert_eq!(
            process_start("42 (a)b c)) S 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 999 20"),
            Some(999)
        );
        assert_eq!(process_start("42 (incomplete) S 1"), None);
    }

    #[test]
    fn partial_write_services_cancellation_without_finishing_bytes() {
        let (writer, mut reader) = UnixStream::pair().unwrap();
        writer.set_nonblocking(true).unwrap();
        reader.set_nonblocking(true).unwrap();
        nix::sys::socket::setsockopt(&writer, sockopt::SndBuf, &1024).unwrap();
        let bytes = vec![7; MAX_FRAME];
        let mut delivered = Vec::new();
        let result = write_all(
            &writer,
            &bytes,
            Instant::now() + Duration::from_secs(2),
            &mut || {
                assert_eq!(
                    reader.read_to_end(&mut delivered).unwrap_err().kind(),
                    io::ErrorKind::WouldBlock
                );
                if delivered.is_empty() {
                    Ok(())
                } else {
                    Err(Failure::Progress(io::ErrorKind::Interrupted.into()))
                }
            },
        );
        assert!(matches!(result, Err(Failure::Progress(_))));
        assert!(!delivered.is_empty() && delivered.len() < bytes.len());
        let mut extra = Vec::new();
        assert_eq!(
            reader.read_to_end(&mut extra).unwrap_err().kind(),
            io::ErrorKind::WouldBlock
        );
        assert!(extra.is_empty());
    }

    #[test]
    fn final_decoded_reply_progress_refusal_or_identity_change_preserves_pending() {
        use nix::sys::socket::{MsgFlags, recv};
        use std::os::unix::{fs::PermissionsExt, net::UnixListener};
        use std::sync::{
            Arc,
            atomic::{AtomicBool, Ordering},
            mpsc,
        };
        use std::thread;
        let id = |n| Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap();
        for select in [false, true] {
            for replace in [false, true] {
                let path = std::env::temp_dir().join(format!(
                    "rsdk-final-progress-{}-{select}-{replace}",
                    std::process::id()
                ));
                fs::create_dir(&path).unwrap();
                fs::set_permissions(&path, fs::Permissions::from_mode(0o700)).unwrap();
                let socket = path.join("sdk.sock");
                let listener = UnixListener::bind(&socket).unwrap();
                fs::set_permissions(&socket, fs::Permissions::from_mode(0o600)).unwrap();
                let ready = Arc::new(AtomicBool::new(false));
                let peer_ready = ready.clone();
                let (sent, received) = mpsc::channel();
                let peer = thread::spawn(move || {
                    let (mut stream, _) = listener.accept().unwrap();
                    stream.set_nonblocking(true).unwrap();
                    let until = Instant::now() + Duration::from_secs(3);
                    let mut hello = read_frame(&stream, until, &mut || Ok(())).unwrap();
                    hello.message = Message::Ready;
                    hello.scope.server_nonce = Some(id(4));
                    write_all(&stream, &hello.encode().unwrap(), until, &mut || Ok(())).unwrap();
                    let request = read_frame(&stream, until, &mut || Ok(())).unwrap();
                    let reply = match &request.message {
                        Message::Request(Command::InsertAfter(value)) => Reply::InsertionObserved {
                            target: PageKey {
                                document: value.source.page().document,
                                page: value.target,
                            },
                            after_order: value.expected_after_order(),
                        },
                        Message::Request(Command::SelectAdjacent(value)) => Reply::TargetObserved(
                            ReaderFacts::new(
                                PageKey {
                                    document: value.expected.page().document,
                                    page: value.target,
                                },
                                value.expected.order().to_vec(),
                            )
                            .unwrap(),
                        ),
                        _ => panic!(),
                    };
                    let response = Frame {
                        message: Message::Response {
                            request_sha256: request.fingerprint().unwrap(),
                            reply,
                        },
                        ..request
                    };
                    peer_ready.store(true, Ordering::Release);
                    write_all(&stream, &response.encode().unwrap(), until, &mut || Ok(())).unwrap();
                    sent.send(()).unwrap();
                    stream.set_nonblocking(false).unwrap();
                    stream
                        .set_read_timeout(Some(Duration::from_secs(3)))
                        .unwrap();
                    assert_eq!(stream.read(&mut [0; 1]).unwrap(), 0);
                });
                let scope = ConnectionIdentity {
                    runtime: RuntimeIdentity {
                        profile_sha256: [1; 32],
                        executable_sha256: [2; 32],
                        adapter_sha256: [3; 32],
                        boot: Uuid::parse(
                            bounded_text("/proc/sys/kernel/random/boot_id")
                                .unwrap()
                                .trim(),
                        )
                        .unwrap(),
                        pid: std::process::id(),
                        start_ticks: process_start(&bounded_text("/proc/self/stat").unwrap())
                            .unwrap(),
                        activation: id(2),
                    },
                    connection: id(3),
                    server_nonce: None,
                };
                let mut client = Client::connect(
                    File::open(&path).unwrap(),
                    fs::metadata(&path).unwrap().uid(),
                    scope,
                    OperationId(id(5)),
                    Instant::now() + Duration::from_secs(2),
                )
                .unwrap();
                // Private test inspection only: no duplicate socket or raw access
                // is exposed by Client's public interface.
                let fd = client.stream.as_raw_fd();
                let source = ReaderFacts::new(
                    PageKey {
                        document: id(10),
                        page: id(11),
                    },
                    vec![id(11), id(12)],
                )
                .unwrap();
                let command = if select {
                    Command::SelectAdjacent(SelectAdjacent {
                        expected: source,
                        target: id(12),
                    })
                } else {
                    Command::InsertAfter(InsertAfter {
                        source,
                        target: id(13),
                    })
                };
                let mut available = false;
                let mut final_checked = false;
                let mut original = None;
                let error = client
                    .execute_with_progress(
                        OperationId(id(20)),
                        command,
                        Instant::now() + Duration::from_secs(2),
                        |frame, digest| {
                            original = Some((frame.operation, digest));
                            Ok(())
                        },
                        &mut || {
                            if ready.load(Ordering::Acquire) {
                                if !available {
                                    received.recv_timeout(Duration::from_secs(1)).unwrap();
                                    available = true;
                                }
                                match recv(
                                    fd,
                                    &mut [0; 1],
                                    MsgFlags::MSG_PEEK | MsgFlags::MSG_DONTWAIT,
                                ) {
                                    Ok(1) => {} // Complete response is buffered; let reads finish.
                                    Err(Errno::EAGAIN) => {
                                        // All prefix/body bytes were consumed. This is
                                        // final admission after read_frame/decode.
                                        final_checked = true;
                                        if replace {
                                            fs::set_permissions(
                                                &socket,
                                                fs::Permissions::from_mode(0o666),
                                            )
                                            .unwrap();
                                        } else {
                                            return Err(io::ErrorKind::Interrupted.into());
                                        }
                                    }
                                    other => panic!("unexpected pending bytes: {other:?}"),
                                }
                            }
                            Ok(())
                        },
                    )
                    .unwrap_err();
                assert!(final_checked);
                assert!(if replace {
                    matches!(error.cause, Failure::Identity)
                } else {
                    matches!(error.cause, Failure::Progress(_))
                });
                let (operation, request_sha256) = original.unwrap();
                let expected = LostRequest::Indeterminate {
                    operation,
                    request_sha256,
                };
                assert_eq!(error.lost, expected);
                assert_eq!(client.close(), expected);
                peer.join().unwrap();
                fs::remove_file(socket).unwrap();
                fs::remove_dir(path).unwrap();
            }
        }
    }
}
