//! Owned Qt endpoint interoperability; never loads or contacts a native Reader.
#![cfg(all(unix, feature = "native-adapter"))]
use remarkable_open_sdk::native_adapter::*;
use remarkable_open_sdk::{OperationId, Uuid};
use std::io::{BufRead, BufReader, Read, Write};
use std::os::unix::net::UnixStream;
use std::process::{Child, Command as Process, Stdio};
use std::time::Duration;

struct OwnedServer(Child);
impl Drop for OwnedServer {
    fn drop(&mut self) {
        if let Some(mut input) = self.0.stdin.take() {
            let _ = input.write_all(b"Q");
        }
        for _ in 0..100 {
            if matches!(self.0.try_wait(), Ok(Some(_))) {
                return;
            }
            std::thread::sleep(Duration::from_millis(10));
        }
        let _ = self.0.kill();
        let _ = self.0.wait();
    }
}
fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}
fn read_frame(reader: &mut impl Read) -> std::io::Result<Frame> {
    let mut prefix = [0; 4];
    reader.read_exact(&mut prefix)?;
    let size = u32::from_be_bytes(prefix) as usize;
    assert!((1..=MAX_FRAME).contains(&size));
    let mut payload = vec![0; size];
    reader.read_exact(&mut payload)?;
    Ok(Frame::decode(&payload).unwrap())
}
fn connect(path: &str, scope: ConnectionIdentity) -> (UnixStream, Session, Frame) {
    let mut socket = UnixStream::connect(format!("{path}/sdk.sock")).unwrap();
    socket
        .set_read_timeout(Some(Duration::from_secs(2)))
        .unwrap();
    socket
        .set_write_timeout(Some(Duration::from_secs(2)))
        .unwrap();
    let mut session = Session::new(scope, OperationId(id(50))).unwrap();
    socket
        .write_all(&session.hello().unwrap().encode().unwrap())
        .unwrap();
    let ready = read_frame(&mut socket).unwrap();
    session.accept_ready(&ready).unwrap();
    (socket, session, ready)
}
#[test]
#[ignore = "requires the separately built owned Qt endpoint binary and Rust wire corpus"]
fn rust_session_against_qt_endpoint() {
    let binary = std::env::var_os("RESIDENT_ENDPOINT_BINARY").unwrap();
    let vectors = std::env::var_os("RESIDENT_WIRE_VECTORS").unwrap();
    let mut server = OwnedServer(
        Process::new(binary)
            .arg("--rust-server")
            .arg(std::process::id().to_string())
            .arg(vectors)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .spawn()
            .unwrap(),
    );
    let mut output = BufReader::new(server.0.stdout.take().unwrap());
    let mut path = String::new();
    output.read_line(&mut path).unwrap();
    let identity = read_frame(&mut output).unwrap(); // Fixture bootstrap, not production attestation.
    assert_eq!(identity.scope.runtime.pid, server.0.id());
    let scope = identity.scope;
    let (mut socket, mut session, ready) = connect(path.trim(), scope.clone());
    let mut observed = None;
    for n in 1..=200 {
        let request = session
            .begin(OperationId(id(100 + n)), Command::Observe)
            .unwrap();
        let bytes = request.encode().unwrap();
        for fragment in bytes.chunks(3) {
            socket.write_all(fragment).unwrap();
        }
        let reply = session.complete(&read_frame(&mut socket).unwrap()).unwrap();
        let Reply::Observed(facts) = reply else {
            panic!("expected observation")
        };
        observed = Some(facts);
    }
    let mutation = session
        .begin(
            OperationId(id(900)),
            Command::InsertAfter(InsertAfter {
                source: observed.unwrap(),
                target: id(99),
            }),
        )
        .unwrap();
    socket.write_all(&mutation.encode().unwrap()).unwrap();
    // The fixture intentionally retains this plain request until its deadline.
    assert_eq!(
        read_frame(&mut socket).unwrap_err().kind(),
        std::io::ErrorKind::UnexpectedEof
    );
    assert!(matches!(session.close(), LostRequest::Indeterminate { .. }));
    let (mut next, _, next_ready) = connect(path.trim(), scope);
    assert_ne!(ready.scope.server_nonce, next_ready.scope.server_nonce);
    next.write_all(&mutation.encode().unwrap()).unwrap();
    assert_eq!(
        read_frame(&mut next).unwrap_err().kind(),
        std::io::ErrorKind::UnexpectedEof
    );
    server.0.stdin.take().unwrap().write_all(b"Q").unwrap();
    let mut count = String::new();
    output.read_line(&mut count).unwrap();
    assert_eq!(count, "submissions=201\n"); // Old request never reached the backend.
    assert!(server.0.wait().unwrap().success());
}
