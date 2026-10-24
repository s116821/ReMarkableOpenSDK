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
use std::thread::{self, JoinHandle};
use std::time::{Duration, Instant};

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
fn journal_failure_expiry_and_socket_replacement_send_no_mutation() {
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
            .execute(OperationId(id(999)), mutation(), end, |_, _| match case {
                0 => Err(io::Error::other("owned journal refusal")),
                1 => {
                    thread::sleep(Duration::from_millis(60));
                    Ok(())
                }
                _ => {
                    fs::remove_file(f.path.join("sdk.sock"))?;
                    let replacement = UnixListener::bind(f.path.join("sdk.sock"))?;
                    fs::set_permissions(f.path.join("sdk.sock"), Permissions::from_mode(0o600))?;
                    drop(replacement);
                    Ok(())
                }
            })
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

#[test]
fn eof_mismatch_indeterminate_and_partial_response_preserve_original_loss() {
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
        let result = client.execute(OperationId(id(999)), mutation(), deadline(), |_, hash| {
            digest = hash;
            Ok(())
        });
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
                    mutation(),
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

#[test]
fn slow_fragment_progress_does_not_extend_deadline() {
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
            mutation(),
            start + Duration::from_millis(80),
            |_, _| Ok(()),
        )
        .unwrap_err();
    assert!(matches!(error.cause, Failure::Deadline));
    assert!(matches!(error.lost, LostRequest::Indeterminate { .. }));
    assert!(start.elapsed() < Duration::from_millis(500));
    peer.join().unwrap();
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
