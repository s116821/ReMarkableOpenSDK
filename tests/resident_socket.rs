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

#[cfg(target_os = "linux")]
#[test]
#[ignore = "requires the owned Qt source capture binary; no native Reader or device"]
fn authenticated_capture_from_actual_qt_producer() {
    use remarkable_open_sdk::native_adapter::transport::Client;
    use std::fs::File;
    use std::os::unix::fs::MetadataExt;
    use std::time::Instant;
    for mode in ["success", "reorder", "visual"] {
        let mut server = OwnedServer(
            Process::new(std::env::var_os("READER_SOURCE_CAPTURE_BINARY").unwrap())
                .args(["--rust-server", &std::process::id().to_string(), mode])
                .env("QT_QPA_PLATFORM", "offscreen")
                .env("QT_QUICK_BACKEND", "software")
                .env("ASAN_OPTIONS", "detect_leaks=0:halt_on_error=1")
                .env("UBSAN_OPTIONS", "halt_on_error=1")
                .stdin(Stdio::piped())
                .stdout(Stdio::piped())
                .spawn()
                .unwrap(),
        );
        let mut output = BufReader::new(server.0.stdout.take().unwrap());
        let mut path = String::new();
        output.read_line(&mut path).unwrap();
        let identity = read_frame(&mut output).unwrap();
        assert_eq!(identity.scope.runtime.pid, server.0.id());
        let directory = File::open(path.trim()).unwrap();
        let uid = directory.metadata().unwrap().uid();
        let mut client = Client::connect(
            directory,
            uid,
            identity.scope,
            OperationId(id(70)),
            Instant::now() + Duration::from_secs(3),
        )
        .unwrap();
        let Reply::Observed(source) = client
            .execute(
                OperationId(id(71)),
                Command::Observe,
                Instant::now() + Duration::from_secs(3),
                |_, _| Ok(()),
            )
            .unwrap()
        else {
            panic!()
        };
        let reply = client
            .execute(
                OperationId(id(72)),
                if mode == "visual" {
                    Command::CaptureVisual
                } else {
                    Command::CaptureSource(source.clone())
                },
                Instant::now() + Duration::from_secs(5),
                |_, _| Ok(()),
            )
            .unwrap();
        if mode != "reorder" {
            let capture = match reply {
                Reply::Captured(capture) if mode == "success" => {
                    assert_eq!(capture.before(), &source);
                    assert_eq!(capture.after(), &source);
                    HistoricalVisual::new(
                        capture.dimensions(),
                        capture.device_pixel_ratio(),
                        capture.png().to_vec(),
                        capture.sha256(),
                    )
                    .unwrap()
                }
                Reply::VisualCaptured(capture) if mode == "visual" => capture,
                _ => panic!("actual Qt capture expected"),
            };
            assert_eq!(capture.dimensions(), [160, 120]);
            assert_eq!(capture.device_pixel_ratio(), 1.0);
            let decoded =
                image::load_from_memory_with_format(capture.png(), image::ImageFormat::Png)
                    .unwrap()
                    .to_rgb8();
            let mut state = 0x12345678u32;
            for pixel in decoded.pixels() {
                state ^= state << 13;
                state ^= state >> 17;
                state ^= state << 5;
                assert_eq!(
                    pixel.0,
                    [state as u8, (state >> 8) as u8, (state >> 16) as u8]
                );
            }
            if let Some(path) = std::env::var_os("RSDK_CAPTURE_IMAGE_OUTPUT") {
                let mut artifact = std::fs::OpenOptions::new()
                    .write(true)
                    .create_new(true)
                    .open(if mode == "visual" {
                        std::path::PathBuf::from(path).with_extension("visual.png")
                    } else {
                        path.into()
                    })
                    .unwrap();
                artifact.write_all(capture.png()).unwrap();
            }
        } else {
            assert_eq!(reply, Reply::Rejected(Rejection::UnknownIdentity));
        }
        client.close();
        server.0.stdin.take().unwrap().write_all(b"Q").unwrap();
        let mut count = String::new();
        output.read_line(&mut count).unwrap();
        assert_eq!(
            count,
            if mode == "visual" {
                "submissions=2 samples=0 grabs=1\n"
            } else {
                "submissions=2 samples=2 grabs=1\n"
            }
        );
        assert!(server.0.wait().unwrap().success());
    }
}
fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}
fn read_frame(reader: &mut impl Read) -> std::io::Result<Frame> {
    let mut prefix = [0; 4];
    reader.read_exact(&mut prefix)?;
    let size = u32::from_be_bytes(prefix) as usize;
    assert!((1..=MAX_CAPTURE_FRAME).contains(&size));
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

#[cfg(target_os = "linux")]
#[test]
#[ignore = "requires the separately built owned Qt endpoint binary and Rust wire corpus"]
fn authenticated_client_against_qt_endpoint() {
    use remarkable_open_sdk::native_adapter::transport::Client;
    use std::fs::File;
    use std::os::unix::fs::MetadataExt;
    use std::time::Instant;
    let mut server = OwnedServer(
        Process::new(std::env::var_os("RESIDENT_ENDPOINT_BINARY").unwrap())
            .arg("--rust-server")
            .arg(std::process::id().to_string())
            .arg(std::env::var_os("RESIDENT_WIRE_VECTORS").unwrap())
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .spawn()
            .unwrap(),
    );
    let mut output = BufReader::new(server.0.stdout.take().unwrap());
    let mut path = String::new();
    output.read_line(&mut path).unwrap();
    // Owned fixture bootstrap asserts its hashes. Production requires separate
    // supervisor verification; Client independently checks kernel PID/start/boot.
    let identity = read_frame(&mut output).unwrap();
    assert_eq!(identity.scope.runtime.pid, server.0.id());
    let directory = File::open(path.trim()).unwrap();
    let uid = directory.metadata().unwrap().uid();
    let mut client = Client::connect(
        directory,
        uid,
        identity.scope,
        OperationId(id(50)),
        Instant::now() + Duration::from_secs(2),
    )
    .unwrap();
    let Reply::Observed(source) = client
        .execute(
            OperationId(id(51)),
            Command::Observe,
            Instant::now() + Duration::from_secs(2),
            |_, _| Ok(()),
        )
        .unwrap()
    else {
        panic!()
    };
    let selection = SelectAdjacent {
        expected: source.clone(),
        target: source.order()[2],
    };
    let selected = ReaderFacts::new(
        remarkable_open_sdk::PageKey {
            document: source.page().document,
            page: selection.target,
        },
        source.order().to_vec(),
    )
    .unwrap();
    assert_eq!(
        client
            .execute(
                OperationId(id(53)),
                Command::SelectAdjacent(selection.clone()),
                Instant::now() + Duration::from_secs(2),
                |request, hash| {
                    assert_eq!(request.fingerprint().unwrap(), hash);
                    Ok(())
                }
            )
            .unwrap(),
        Reply::TargetObserved(selected)
    );
    assert_eq!(
        client
            .execute(
                OperationId(id(54)),
                Command::ReconcileSelection(selection),
                Instant::now() + Duration::from_secs(2),
                |_, _| Ok(())
            )
            .unwrap(),
        Reply::SelectionReconciled {
            current: source.clone(),
            target_present: true
        }
    );
    let Reply::Captured(capture) = client
        .execute(
            OperationId(id(55)),
            Command::CaptureSource(source.clone()),
            Instant::now() + Duration::from_secs(2),
            |_, _| Ok(()),
        )
        .unwrap()
    else {
        panic!("expected owned historical capture")
    };
    assert_eq!(capture.before(), &source);
    assert_eq!(capture.after(), &source);
    assert_eq!(capture.dimensions(), [160, 120]);
    assert!(capture.png().len() > MAX_FRAME);
    assert_eq!(capture.procedure(), CAPTURE_PROCEDURE);
    let mut digest = [0; 32];
    let error = client
        .execute(
            OperationId(id(52)),
            Command::InsertAfter(InsertAfter {
                source,
                target: id(99),
            }),
            Instant::now() + Duration::from_secs(2),
            |_, hash| {
                digest = hash;
                Ok(())
            },
        )
        .unwrap_err();
    assert_eq!(
        error.lost,
        LostRequest::Indeterminate {
            operation: OperationId(id(52)),
            request_sha256: digest
        }
    );
    assert_eq!(client.close(), error.lost);
    server.0.stdin.take().unwrap().write_all(b"Q").unwrap();
    let mut count = String::new();
    output.read_line(&mut count).unwrap();
    assert_eq!(count, "submissions=5\n");
    assert!(server.0.wait().unwrap().success());
}
