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
        check_deadline(deadline)?;
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
        let pending = match connect(fd.as_raw_fd(), &address) {
            Ok(()) => false,
            Err(Errno::EINPROGRESS) => true,
            // A full Unix listener backlog (EAGAIN) is a refusal, never retried.
            Err(error) => return Err(error.into()),
        };
        let stream = UnixStream::from(fd);
        if pending {
            wait(&stream, PollFlags::POLLOUT, deadline)?;
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
        client.authenticate()?;
        let hello = client.session.hello()?.encode()?;
        write_all(&mut client.stream, &hello, deadline)?;
        let ready = read_frame(&mut client.stream, deadline)?;
        client.authenticate()?;
        check_deadline(deadline)?;
        client.session.accept_ready(&ready)?;
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
        let result = (|| {
            check_deadline(deadline)?;
            self.authenticate()?;
            let prepared = self.session.prepare(operation, command.clone())?;
            let bytes = prepared.encode()?;
            let digest = prepared.fingerprint()?;
            journal(&prepared, digest).map_err(Failure::Journal)?;
            // Journaling may be slow or replace filesystem entries. Recheck.
            self.authenticate()?;
            check_deadline(deadline)?;
            self.session.begin(operation, command)?;
            write_all(&mut self.stream, &bytes, deadline)?;
            let response = read_frame(&mut self.stream, deadline)?;
            self.authenticate()?;
            check_deadline(deadline)?;
            let reply = self.session.complete(&response)?;
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

    fn authenticate(&self) -> TransportResult<()> {
        check_directory(&self.directory, self.uid)?;
        let metadata = fs::symlink_metadata(socket_path(&self.directory))?;
        check_socket(&metadata, self.uid)?;
        if (metadata.dev(), metadata.ino()) != self.socket_identity {
            return Err(Failure::Identity);
        }
        let peer = getsockopt(&self.stream, sockopt::PeerCredentials)?;
        if peer.pid() <= 0 || peer.pid() as u32 != self.runtime.pid || peer.uid() != self.uid {
            return Err(Failure::Identity);
        }
        let boot = bounded_text("/proc/sys/kernel/random/boot_id")?;
        let stat = bounded_text(&format!("/proc/{}/stat", self.runtime.pid))?;
        if Uuid::parse(boot.trim()) != Some(self.runtime.boot)
            || process_start(&stat) != Some(self.runtime.start_ticks)
        {
            return Err(Failure::Identity);
        }
        Ok(())
    }
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
fn wait(stream: &UnixStream, flags: PollFlags, deadline: Instant) -> TransportResult<()> {
    loop {
        check_deadline(deadline)?;
        let ms = deadline
            .saturating_duration_since(Instant::now())
            .as_millis()
            .saturating_add(1)
            .min(i32::MAX as u128);
        let mut fds = [PollFd::new(stream.as_fd(), flags)];
        match poll(
            &mut fds,
            PollTimeout::try_from(ms).expect("bounded timeout"),
        ) {
            Ok(0) | Err(Errno::EINTR) => continue,
            Ok(_) => {
                check_deadline(deadline)?;
                return Ok(());
            }
            Err(error) => return Err(error.into()),
        }
    }
}
fn write_all(stream: &mut UnixStream, mut bytes: &[u8], deadline: Instant) -> TransportResult<()> {
    while !bytes.is_empty() {
        wait(stream, PollFlags::POLLOUT, deadline)?;
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
    stream: &mut UnixStream,
    mut bytes: &mut [u8],
    deadline: Instant,
) -> TransportResult<()> {
    while !bytes.is_empty() {
        wait(stream, PollFlags::POLLIN, deadline)?;
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
fn read_frame(stream: &mut UnixStream, deadline: Instant) -> TransportResult<Frame> {
    let mut prefix = [0; 4];
    read_exact(stream, &mut prefix, deadline)?;
    let size = u32::from_be_bytes(prefix) as usize;
    if size == 0 || size > MAX_FRAME {
        return Err(ProtocolError::TooLarge.into());
    }
    let mut bytes = vec![0; size];
    read_exact(stream, &mut bytes, deadline)?;
    Ok(Frame::decode(&bytes)?)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::Duration;

    #[test]
    fn partial_write_expires_without_reset_or_full_delivery() {
        let (mut writer, mut reader) = UnixStream::pair().unwrap();
        writer.set_nonblocking(true).unwrap();
        reader.set_nonblocking(true).unwrap();
        nix::sys::socket::setsockopt(&writer, sockopt::SndBuf, &1024).unwrap();
        let bytes = vec![7; MAX_FRAME];
        assert!(matches!(
            write_all(
                &mut writer,
                &bytes,
                Instant::now() + Duration::from_millis(30)
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
}
