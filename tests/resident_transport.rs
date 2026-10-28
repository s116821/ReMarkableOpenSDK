//! Real Linux sockets/process credentials with owned peers; no native runtime.
#![cfg(all(target_os = "linux", feature = "native-adapter"))]
use remarkable_open_sdk::native_adapter::{transport::*, *};
use remarkable_open_sdk::{OperationId, PageKey, Uuid};
use std::fs::{self, File, Permissions};
use std::io::{self, Read, Write};
use std::os::unix::fs::{MetadataExt, PermissionsExt};
use std::os::unix::net::{UnixListener, UnixStream};
use std::path::PathBuf;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::{Arc, atomic::AtomicBool, mpsc};
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};
#[path = "support/resident_capture.rs"]
mod capture_support;

#[test]
fn capture_delivery_validates_image_source_and_wire_correspondence() {
    for fault in 0..7 {
        let f = Fixture::new();
        let peer = f.peer(move |mut stream| {
            let request = read_frame(&mut stream);
            let mut source = facts();
            if fault == 5 {
                source = ReaderFacts::new(
                    PageKey {
                        document: id(99),
                        page: source.page().page,
                    },
                    source.order().to_vec(),
                )
                .unwrap();
            }
            let capture = capture_support::capture(source, 160, 120);
            assert!(capture.png().len() > MAX_FRAME);
            let mut frame = response(&request, Reply::Captured(capture));
            if fault == 4 {
                frame.operation = OperationId(id(999));
            }
            let mut bytes = frame.encode().unwrap();
            match fault {
                1 => {
                    let end = bytes.len() - 30;
                    bytes[end] ^= 1;
                }
                2 => {
                    bytes.truncate(bytes.len() - 1);
                }
                3 => bytes = ((MAX_CAPTURE_FRAME + 1) as u32).to_be_bytes().to_vec(),
                6 => {
                    bytes = response(&request, Reply::Observed(facts()))
                        .encode()
                        .unwrap();
                    bytes.resize(MAX_FRAME + 5, 0);
                    let size = (bytes.len() - 4) as u32;
                    bytes[..4].copy_from_slice(&size.to_be_bytes());
                }
                _ => {}
            }
            for part in bytes.chunks(4093) {
                if stream.write_all(part).is_err() {
                    break;
                }
            }
        });
        let mut client = f.connect(scope()).unwrap();
        let result = client.execute(
            OperationId(id(60)),
            Command::CaptureSource(facts()),
            deadline(),
            |_, _| Ok(()),
        );
        if fault == 0 {
            let Reply::Captured(capture) = result.unwrap() else {
                panic!()
            };
            assert_eq!(capture.before(), &facts());
            assert_eq!(capture.after(), &facts());
            assert_eq!(capture.dimensions(), [160, 120]);
        } else {
            assert_eq!(result.unwrap_err().lost, LostRequest::NoPendingMutation);
        }
        peer.join().unwrap();
    }
}

#[test]
fn partial_capture_cancel_or_deadline_discards_delivery() {
    for cancel in [true, false] {
        let f = Fixture::new();
        let partial = Arc::new(AtomicBool::new(false));
        let sent = partial.clone();
        // Encode before starting the request's deliberately short budget.
        let capture = capture_support::capture(facts(), 160, 120);
        let peer = f.peer(move |mut stream| {
            let request = read_frame(&mut stream);
            let bytes = response(&request, Reply::Captured(capture))
                .encode()
                .unwrap();
            stream.write_all(&bytes[..1024]).unwrap();
            sent.store(true, Ordering::Release);
            thread::sleep(Duration::from_millis(200));
            let _ = stream.write_all(&bytes[1024..]);
        });
        let mut client = f.connect(scope()).unwrap();
        let error = client
            .execute_with_progress(
                OperationId(id(61)),
                Command::CaptureSource(facts()),
                Instant::now() + Duration::from_millis(if cancel { 1000 } else { 80 }),
                |_, _| Ok(()),
                &mut || {
                    if cancel && partial.load(Ordering::Acquire) {
                        Err(io::Error::other("capture canceled"))
                    } else {
                        Ok(())
                    }
                },
            )
            .unwrap_err();
        if cancel {
            assert!(matches!(error.cause, Failure::Progress(_)));
        } else {
            assert!(matches!(error.cause, Failure::Deadline));
        }
        assert_eq!(error.lost, LostRequest::NoPendingMutation);
        peer.join().unwrap();
    }
}

fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}
fn deadline() -> Instant {
    Instant::now() + Duration::from_secs(2)
}
fn scope() -> ConnectionIdentity {
    let stat = fs::read_to_string("/proc/self/stat").unwrap();
    ConnectionIdentity {
        runtime: RuntimeIdentity {
            profile_sha256: [1; 32],
            executable_sha256: [2; 32],
            adapter_sha256: [3; 32],
            boot: Uuid::parse(
                fs::read_to_string("/proc/sys/kernel/random/boot_id")
                    .unwrap()
                    .trim(),
            )
            .unwrap(),
            pid: std::process::id(),
            start_ticks: stat
                .rsplit_once(')')
                .unwrap()
                .1
                .split_whitespace()
                .nth(19)
                .unwrap()
                .parse()
                .unwrap(),
            activation: id(2),
        },
        connection: id(3),
        server_nonce: None,
    }
}
fn facts() -> ReaderFacts {
    ReaderFacts::new(
        PageKey {
            document: id(10),
            page: id(11),
        },
        vec![id(11), id(12)],
    )
    .unwrap()
}
fn mutation() -> Command {
    Command::InsertAfter(InsertAfter {
        source: facts(),
        target: id(13),
    })
}
fn effects() -> [Command; 2] {
    [
        mutation(),
        Command::SelectAdjacent(SelectAdjacent {
            expected: facts(),
            target: id(12),
        }),
    ]
}
fn read_frame(stream: &mut UnixStream) -> Frame {
    let mut prefix = [0; 4];
    stream.read_exact(&mut prefix).unwrap();
    let size = u32::from_be_bytes(prefix) as usize;
    assert!((1..=MAX_FRAME).contains(&size));
    let mut bytes = vec![0; size];
    stream.read_exact(&mut bytes).unwrap();
    Frame::decode(&bytes).unwrap()
}
fn response(request: &Frame, reply: Reply) -> Frame {
    Frame {
        message: Message::Response {
            request_sha256: request.fingerprint().unwrap(),
            reply,
        },
        ..request.clone()
    }
}
fn write_frame(stream: &mut UnixStream, frame: &Frame) {
    stream.write_all(&frame.encode().unwrap()).unwrap();
}
struct Fixture {
    path: PathBuf,
    listener: UnixListener,
    uid: u32,
}
impl Fixture {
    fn new() -> Self {
        static NEXT: AtomicUsize = AtomicUsize::new(0);
        let path = std::env::temp_dir().join(format!(
            "rsdk-client-{}-{}",
            std::process::id(),
            NEXT.fetch_add(1, Ordering::Relaxed)
        ));
        fs::create_dir(&path).unwrap();
        fs::set_permissions(&path, Permissions::from_mode(0o700)).unwrap();
        let listener = UnixListener::bind(path.join("sdk.sock")).unwrap();
        fs::set_permissions(path.join("sdk.sock"), Permissions::from_mode(0o600)).unwrap();
        let uid = fs::metadata(&path).unwrap().uid();
        Self {
            path,
            listener,
            uid,
        }
    }
    fn connect(&self, scope: ConnectionIdentity) -> Result<Client, Failure> {
        Client::connect(
            File::open(&self.path).unwrap(),
            self.uid,
            scope,
            OperationId(id(5)),
            deadline(),
        )
    }
    fn peer(&self, callback: impl FnOnce(UnixStream) + Send + 'static) -> JoinHandle<()> {
        let listener = self.listener.try_clone().unwrap();
        thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            stream
                .set_read_timeout(Some(Duration::from_secs(3)))
                .unwrap();
            stream
                .set_write_timeout(Some(Duration::from_secs(3)))
                .unwrap();
            let mut hello = read_frame(&mut stream);
            assert_eq!(hello.message, Message::Hello);
            hello.message = Message::Ready;
            hello.scope.server_nonce = Some(id(4));
            write_frame(&mut stream, &hello);
            callback(stream);
        })
    }
}
impl Drop for Fixture {
    fn drop(&mut self) {
        let _ = fs::remove_file(self.path.join("sdk.sock"));
        let _ = fs::remove_dir(&self.path);
    }
}

#[test]
fn authenticated_requests_journal_exact_frame_and_reuse_connection() {
    let f = Fixture::new();
    let peer = f.peer(|mut stream| {
        for n in 1..=200 {
            let request = read_frame(&mut stream);
            assert_eq!(request.sequence, n);
            assert_eq!(request.operation, OperationId(id(100 + n as u32)));
            let answer = response(&request, Reply::Observed(facts()))
                .encode()
                .unwrap();
            for part in answer.chunks(3) {
                stream.write_all(part).unwrap();
            }
        }
        let request = read_frame(&mut stream);
        let Command::InsertAfter(value) = (match &request.message {
            Message::Request(c) => c,
            _ => panic!(),
        }) else {
            panic!()
        };
        write_frame(
            &mut stream,
            &response(
                &request,
                Reply::InsertionObserved {
                    target: PageKey {
                        document: value.source.page().document,
                        page: value.target,
                    },
                    after_order: value.expected_after_order(),
                },
            ),
        );
        let mut eof = [0];
        assert_eq!(stream.read(&mut eof).unwrap(), 0);
    });
    let mut client = f.connect(scope()).unwrap();
    for n in 1..=200 {
        assert_eq!(
            client
                .execute(
                    OperationId(id(100 + n)),
                    Command::Observe,
                    deadline(),
                    |_, _| Ok(())
                )
                .unwrap(),
            Reply::Observed(facts())
        );
    }
    let mut journaled = false;
    let reply = client
        .execute(
            OperationId(id(999)),
            mutation(),
            deadline(),
            |request, digest| {
                assert_eq!(request.operation, OperationId(id(999)));
                assert_eq!(request.message, Message::Request(mutation()));
                assert_eq!(request.fingerprint().unwrap(), digest);
                assert_eq!(request.sequence, 201);
                journaled = true;
                Ok(())
            },
        )
        .unwrap();
    assert!(journaled && matches!(reply, Reply::InsertionObserved { .. }));
    assert_eq!(client.close(), LostRequest::NoPendingMutation);
    peer.join().unwrap();
}

#[test]
fn wrong_kernel_process_boot_or_start_sends_no_hello() {
    for field in 0..3 {
        let f = Fixture::new();
        let mut identity = scope();
        match field {
            0 => identity.runtime.pid += 1,
            1 => identity.runtime.boot = id(99),
            _ => identity.runtime.start_ticks += 1,
        }
        assert!(matches!(f.connect(identity), Err(Failure::Identity)));
        let (mut accepted, _) = f.listener.accept().unwrap();
        let mut bytes = [0];
        assert_eq!(accepted.read(&mut bytes).unwrap(), 0);
    }
}

#[test]
fn directory_socket_modes_owner_and_symlink_refused() {
    let f = Fixture::new();
    fs::set_permissions(&f.path, Permissions::from_mode(0o750)).unwrap();
    assert!(matches!(f.connect(scope()), Err(Failure::Identity)));
    fs::set_permissions(&f.path, Permissions::from_mode(0o700)).unwrap();
    fs::set_permissions(f.path.join("sdk.sock"), Permissions::from_mode(0o666)).unwrap();
    assert!(matches!(f.connect(scope()), Err(Failure::Identity)));
    fs::set_permissions(f.path.join("sdk.sock"), Permissions::from_mode(0o600)).unwrap();
    assert!(matches!(
        Client::connect(
            File::open(&f.path).unwrap(),
            f.uid.wrapping_add(1),
            scope(),
            OperationId(id(5)),
            deadline()
        ),
        Err(Failure::Identity)
    ));
    fs::rename(f.path.join("sdk.sock"), f.path.join("original.sock")).unwrap();
    std::os::unix::fs::symlink("original.sock", f.path.join("sdk.sock")).unwrap();
    assert!(matches!(f.connect(scope()), Err(Failure::Identity)));
    fs::remove_file(f.path.join("original.sock")).unwrap();
}

#[test]
fn retained_directory_descriptor_survives_path_replacement() {
    let mut f = Fixture::new();
    let old_path = f.path.clone();
    let directory = File::open(&f.path).unwrap();
    f.path = f.path.with_extension("retained");
    fs::rename(&old_path, &f.path).unwrap();
    fs::create_dir(&old_path).unwrap(); // Has no socket; must not be followed.
    let peer = f.peer(|mut stream| {
        let request = read_frame(&mut stream);
        write_frame(&mut stream, &response(&request, Reply::Observed(facts())));
    });
    let mut client =
        Client::connect(directory, f.uid, scope(), OperationId(id(5)), deadline()).unwrap();
    assert_eq!(
        client
            .execute(OperationId(id(6)), Command::Observe, deadline(), |_, _| Ok(
                ()
            ))
            .unwrap(),
        Reply::Observed(facts())
    );
    peer.join().unwrap();
    fs::remove_dir(old_path).unwrap();
}

#[test]
fn journal_failure_expiry_and_socket_replacement_send_no_effect() {
    for effect in effects() {
        for case in 0..3 {
            let f = Fixture::new();
            let peer = f.peer(|mut stream| {
                let mut byte = [0];
                assert_eq!(stream.read(&mut byte).unwrap(), 0);
            });
            let mut client = f.connect(scope()).unwrap();
            let end = if case == 1 {
                Instant::now() + Duration::from_millis(30)
            } else {
                deadline()
            };
            let error = client
                .execute(
                    OperationId(id(999)),
                    effect.clone(),
                    end,
                    |_, _| match case {
                        0 => Err(io::Error::other("owned journal refusal")),
                        1 => {
                            thread::sleep(Duration::from_millis(60));
                            Ok(())
                        }
                        _ => {
                            fs::remove_file(f.path.join("sdk.sock"))?;
                            let replacement = UnixListener::bind(f.path.join("sdk.sock"))?;
                            fs::set_permissions(
                                f.path.join("sdk.sock"),
                                Permissions::from_mode(0o600),
                            )?;
                            drop(replacement);
                            Ok(())
                        }
                    },
                )
                .unwrap_err();
            assert_eq!(error.lost, LostRequest::NoPendingMutation);
            match case {
                0 => assert!(matches!(error.cause, Failure::Journal(_))),
                1 => assert!(matches!(error.cause, Failure::Deadline)),
                _ => assert!(matches!(error.cause, Failure::Identity)),
            }
            peer.join().unwrap();
        }
    }
}

#[test]
fn eof_mismatch_indeterminate_and_partial_response_preserve_original_loss() {
    for effect in effects() {
        for case in 0..5 {
            let f = Fixture::new();
            let peer = f.peer(move |mut stream| {
                let request = read_frame(&mut stream);
                let mut reply = response(&request, Reply::Indeterminate);
                match case {
                    0 => {} // EOF after dispatch
                    1 => {
                        reply.operation = OperationId(id(9999));
                        write_frame(&mut stream, &reply);
                    }
                    2 => write_frame(&mut stream, &reply),
                    3 => {
                        stream.write_all(&reply.encode().unwrap()[..12]).unwrap();
                    }
                    _ => {
                        stream
                            .write_all(&((MAX_FRAME as u32) + 1).to_be_bytes())
                            .unwrap();
                    }
                }
            });
            let mut client = f.connect(scope()).unwrap();
            let mut digest = [0; 32];
            let result = client.execute(
                OperationId(id(999)),
                effect.clone(),
                deadline(),
                |_, hash| {
                    digest = hash;
                    Ok(())
                },
            );
            let expected = LostRequest::Indeterminate {
                operation: OperationId(id(999)),
                request_sha256: digest,
            };
            if case == 2 {
                assert_eq!(result.unwrap(), Reply::Indeterminate);
            } else {
                assert_eq!(result.unwrap_err().lost, expected);
            }
            assert_eq!(client.close(), expected);
            assert_eq!(
                client
                    .execute(
                        OperationId(id(1000)),
                        effect.clone(),
                        deadline(),
                        |_, _| panic!("closed must not journal")
                    )
                    .unwrap_err()
                    .lost,
                expected
            );
            peer.join().unwrap();
        }
    }
}

#[test]
fn slow_fragment_progress_does_not_extend_deadline() {
    for effect in effects() {
        let f = Fixture::new();
        let peer = f.peer(|mut stream| {
            let request = read_frame(&mut stream);
            let bytes = response(&request, Reply::Indeterminate).encode().unwrap();
            for byte in bytes {
                if stream.write_all(&[byte]).is_err() {
                    break;
                }
                thread::sleep(Duration::from_millis(10));
            }
        });
        let mut client = f.connect(scope()).unwrap();
        let start = Instant::now();
        let error = client
            .execute(
                OperationId(id(999)),
                effect.clone(),
                start + Duration::from_millis(80),
                |_, _| Ok(()),
            )
            .unwrap_err();
        assert!(matches!(error.cause, Failure::Deadline));
        assert!(matches!(error.lost, LostRequest::Indeterminate { .. }));
        assert!(start.elapsed() < Duration::from_millis(500));
        peer.join().unwrap();
    }
}

#[test]
fn handshake_timeout_is_bounded() {
    let f = Fixture::new();
    assert!(matches!(
        Client::connect(
            File::open(&f.path).unwrap(),
            f.uid,
            scope(),
            OperationId(id(5)),
            Instant::now() + Duration::from_millis(30)
        ),
        Err(Failure::Deadline)
    ));
}

#[test]
fn selection_is_journaled_before_socket_bytes_and_returns_exact_target() {
    let f = Fixture::new();
    let published = Arc::new(AtomicBool::new(false));
    let seen = published.clone();
    let peer = f.peer(move |mut stream| {
        let request = read_frame(&mut stream);
        assert!(seen.load(Ordering::Acquire));
        let Message::Request(Command::SelectAdjacent(selection)) = &request.message else {
            panic!()
        };
        assert_eq!(selection.target, id(12));
        let selected = ReaderFacts::new(
            PageKey {
                document: id(10),
                page: selection.target,
            },
            selection.expected.order().to_vec(),
        )
        .unwrap();
        write_frame(
            &mut stream,
            &response(&request, Reply::TargetObserved(selected)),
        );
    });
    let mut client = f.connect(scope()).unwrap();
    let reply = client
        .execute(
            OperationId(id(1001)),
            effects()[1].clone(),
            deadline(),
            |frame, digest| {
                assert_eq!(frame.fingerprint().unwrap(), digest);
                assert_eq!(frame.operation, OperationId(id(1001)));
                assert_eq!(frame.message, Message::Request(effects()[1].clone()));
                published.store(true, Ordering::Release);
                Ok(())
            },
        )
        .unwrap();
    assert_eq!(
        reply,
        Reply::TargetObserved(
            ReaderFacts::new(
                PageKey {
                    document: id(10),
                    page: id(12)
                },
                vec![id(11), id(12)]
            )
            .unwrap()
        )
    );
    assert_eq!(client.close(), LostRequest::NoPendingMutation);
    peer.join().unwrap();
}

#[test]
fn caller_refusal_or_expiry_before_connect_sends_nothing() {
    for expire in [false, true] {
        let f = Fixture::new();
        let until = Instant::now() + Duration::from_millis(30);
        let mut calls = 0;
        let result = Client::connect_with_progress(
            File::open(&f.path).unwrap(),
            f.uid,
            scope(),
            OperationId(id(5)),
            until,
            &mut || {
                calls += 1;
                if expire {
                    thread::sleep(
                        until.saturating_duration_since(Instant::now()) + Duration::from_millis(1),
                    );
                    Ok(())
                } else {
                    Err(io::ErrorKind::Interrupted.into())
                }
            },
        );
        assert_eq!(calls, 1);
        assert!(if expire {
            matches!(result, Err(Failure::Deadline))
        } else {
            matches!(result, Err(Failure::Progress(_)))
        });
        f.listener.set_nonblocking(true).unwrap();
        assert_eq!(
            f.listener.accept().unwrap_err().kind(),
            io::ErrorKind::WouldBlock
        );
    }
}

#[test]
fn stalled_or_fragmented_handshake_services_cancellation() {
    for fragment in [false, true] {
        let f = Fixture::new();
        let listener = f.listener.try_clone().unwrap();
        let (sent, received) = mpsc::channel();
        let peer = thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            stream
                .set_read_timeout(Some(Duration::from_secs(3)))
                .unwrap();
            let hello = read_frame(&mut stream);
            assert_eq!(hello.message, Message::Hello);
            if fragment {
                stream.write_all(&[0, 0]).unwrap();
            }
            sent.send(()).unwrap();
            match stream.read(&mut [0; 1]) {
                Ok(0) => {}
                Err(error) if error.kind() == io::ErrorKind::ConnectionReset => {}
                other => panic!("unexpected peer result: {other:?}"),
            }
        });
        let started = Instant::now();
        let result = Client::connect_with_progress(
            File::open(&f.path).unwrap(),
            f.uid,
            scope(),
            OperationId(id(5)),
            deadline(),
            &mut || {
                if received.try_recv().is_ok() {
                    Err(io::ErrorKind::Interrupted.into())
                } else {
                    Ok(())
                }
            },
        );
        assert!(matches!(result, Err(Failure::Progress(_))));
        assert!(started.elapsed() < Duration::from_secs(1));
        peer.join().unwrap();
    }
}

#[test]
fn progress_after_journal_refuses_cancellation_expiry_or_replacement_before_bytes() {
    use std::cell::Cell;
    for command in effects() {
        for fault in 0..3 {
            let f = Fixture::new();
            let peer = f.peer(|mut stream| assert_eq!(stream.read(&mut [0; 1]).unwrap(), 0));
            let mut client = f.connect(scope()).unwrap();
            let journaled = Cell::new(false);
            let until = Instant::now() + Duration::from_millis(150);
            let mut original = None;
            let mut refused = false;
            let error = client
                .execute_with_progress(
                    OperationId(id(1100)),
                    command.clone(),
                    until,
                    |frame, digest| {
                        original = Some((frame.clone(), digest));
                        journaled.set(true);
                        Ok(())
                    },
                    &mut || {
                        if journaled.get() {
                            assert!(!refused, "failed progress must not retry");
                            refused = true;
                            match fault {
                                0 => return Err(io::ErrorKind::Interrupted.into()),
                                1 => thread::sleep(
                                    until.saturating_duration_since(Instant::now())
                                        + Duration::from_millis(1),
                                ),
                                _ => fs::set_permissions(
                                    f.path.join("sdk.sock"),
                                    Permissions::from_mode(0o666),
                                )
                                .unwrap(),
                            }
                        }
                        Ok(())
                    },
                )
                .unwrap_err();
            assert!(original.is_some() && refused);
            assert!(match fault {
                0 => matches!(error.cause, Failure::Progress(_)),
                1 => matches!(error.cause, Failure::Deadline),
                _ => matches!(error.cause, Failure::Identity),
            });
            assert_eq!(error.lost, LostRequest::NoPendingMutation);
            assert_eq!(client.close(), LostRequest::NoPendingMutation);
            peer.join().unwrap();
        }
    }
}

#[test]
fn dispatched_progress_failure_keeps_original_loss_with_stalled_partial_or_complete_reply() {
    for command in effects() {
        for reply_bytes in 0..3 {
            let f = Fixture::new();
            let request_seen = Arc::new(AtomicBool::new(false));
            let seen = request_seen.clone();
            let (sent, received) = mpsc::channel();
            let peer = f.peer(move |mut stream| {
                let request = read_frame(&mut stream);
                seen.store(true, Ordering::Release);
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
                let bytes = response(&request, reply).encode().unwrap();
                match reply_bytes {
                    1 => stream.write_all(&bytes[..3]).unwrap(),
                    2 => stream.write_all(&bytes).unwrap(),
                    _ => {}
                }
                sent.send(()).unwrap();
                // Closing with unread reply bytes may cause ECONNRESET on Linux.
                match stream.read(&mut [0; 1]) {
                    Ok(0) => {}
                    Err(error) if error.kind() == io::ErrorKind::ConnectionReset => {}
                    other => panic!("unexpected peer result: {other:?}"),
                }
            });
            let mut client = f.connect(scope()).unwrap();
            let mut original = None;
            let mut canceled = false;
            let error = client
                .execute_with_progress(
                    OperationId(id(1200)),
                    command.clone(),
                    deadline(),
                    |frame, digest| {
                        original = Some((frame.operation, digest));
                        Ok(())
                    },
                    &mut || {
                        if request_seen.load(Ordering::Acquire) {
                            received.recv_timeout(Duration::from_secs(1)).unwrap();
                            canceled = true;
                            Err(io::ErrorKind::Interrupted.into())
                        } else {
                            Ok(())
                        }
                    },
                )
                .unwrap_err();
            let (operation, request_sha256) = original.unwrap();
            let expected = LostRequest::Indeterminate {
                operation,
                request_sha256,
            };
            assert!(canceled && matches!(error.cause, Failure::Progress(_)));
            assert_eq!(error.lost, expected);
            assert_eq!(client.close(), expected);
            assert_eq!(client.close(), expected);
            peer.join().unwrap();
        }
    }
}

#[test]
fn successful_progress_stays_on_caller_thread_and_reuses_session() {
    let f = Fixture::new();
    let peer = f.peer(|mut stream| {
        for _ in 0..3 {
            let request = read_frame(&mut stream);
            write_frame(&mut stream, &response(&request, Reply::Observed(facts())));
        }
    });
    let caller = thread::current().id();
    let mut calls = 0;
    let mut progress = || {
        assert_eq!(thread::current().id(), caller);
        calls += 1;
        Ok(())
    };
    let mut client = Client::connect_with_progress(
        File::open(&f.path).unwrap(),
        f.uid,
        scope(),
        OperationId(id(5)),
        deadline(),
        &mut progress,
    )
    .unwrap();
    for n in 0..3 {
        assert_eq!(
            client
                .execute_with_progress(
                    OperationId(id(1300 + n)),
                    Command::Observe,
                    deadline(),
                    |_, _| Ok(()),
                    &mut progress
                )
                .unwrap(),
            Reply::Observed(facts())
        );
    }
    assert!(calls >= 4);
    assert_eq!(client.close(), LostRequest::NoPendingMutation);
    peer.join().unwrap();
}
