//! Owned interoperability vectors; no connection or native dispatch.
#![cfg(feature = "native-adapter")]
use remarkable_open_sdk::native_adapter::*;
use remarkable_open_sdk::{OperationId, PageKey, Uuid};
use std::{fs, path::PathBuf};
#[path = "support/resident_capture.rs"]
mod capture_support;

fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}
#[test]
#[ignore = "explicit Rust/Qt interoperability corpus export; requires a fresh output directory"]
fn export_vectors() -> Result<(), Box<dyn std::error::Error>> {
    let path = PathBuf::from(
        std::env::var_os("RSDK_WIRE_VECTORS")
            .ok_or("fresh RSDK_WIRE_VECTORS directory required")?,
    );
    fs::create_dir(&path)?; // Never overwrite an earlier corpus.
    let mut scope = ConnectionIdentity {
        runtime: RuntimeIdentity {
            profile_sha256: [1; 32],
            executable_sha256: [2; 32],
            adapter_sha256: [3; 32],
            boot: id(20),
            pid: 17,
            start_ticks: 450,
            activation: id(21),
        },
        connection: id(22),
        server_nonce: None,
    };
    let mut frames = vec![Frame {
        scope: scope.clone(),
        sequence: 0,
        operation: OperationId(id(23)),
        message: Message::Hello,
    }];
    scope.server_nonce = Some(id(24));
    frames.push(Frame {
        scope: scope.clone(),
        sequence: 0,
        operation: OperationId(id(23)),
        message: Message::Ready,
    });
    let facts = ReaderFacts::new(
        PageKey {
            document: id(10),
            page: id(2),
        },
        vec![id(1), id(2), id(3)],
    )
    .unwrap();
    let insertion = InsertAfter {
        source: facts.clone(),
        target: id(4),
    };
    for command in [
        Command::Observe,
        Command::InsertAfter(insertion.clone()),
        Command::Reconcile(insertion),
        Command::SelectAdjacent(SelectAdjacent {
            expected: facts.clone(),
            target: id(3),
        }),
        Command::ReconcileSelection(SelectAdjacent {
            expected: facts.clone(),
            target: id(3),
        }),
    ] {
        frames.push(Frame {
            scope: scope.clone(),
            sequence: 1,
            operation: OperationId(id(30)),
            message: Message::Request(command),
        });
    }
    let digest = frames[3].fingerprint().unwrap();
    for reply in [
        Reply::Observed(facts.clone()),
        Reply::InsertionObserved {
            target: PageKey {
                document: id(10),
                page: id(4),
            },
            after_order: vec![id(1), id(2), id(4), id(3)],
        },
        Reply::Reconciled {
            current: facts.clone(),
            target_present: false,
        },
        Reply::Rejected(Rejection::Unsupported),
        Reply::Rejected(Rejection::BeforeDispatch),
        Reply::Rejected(Rejection::ExecutorBeforeMutation),
        Reply::Rejected(Rejection::UnknownIdentity),
        Reply::Rejected(Rejection::Capacity),
        Reply::Indeterminate,
        Reply::TargetObserved(
            ReaderFacts::new(
                PageKey {
                    document: id(10),
                    page: id(3),
                },
                facts.order().to_vec(),
            )
            .unwrap(),
        ),
        Reply::SelectionReconciled {
            current: facts.clone(),
            target_present: true,
        },
    ] {
        frames.push(Frame {
            scope: scope.clone(),
            sequence: 1,
            operation: OperationId(id(30)),
            message: Message::Response {
                request_sha256: digest,
                reply,
            },
        });
    }
    let capture_request = Frame {
        scope: scope.clone(),
        sequence: 1,
        operation: OperationId(id(31)),
        message: Message::Request(Command::CaptureSource(facts.clone())),
    };
    let captured = capture_support::capture(facts, 160, 120);
    frames.push(capture_request.clone());
    frames.push(Frame {
        message: Message::Response {
            request_sha256: capture_request.fingerprint().unwrap(),
            reply: Reply::Captured(captured),
        },
        ..capture_request
    });
    let mut large = frames[19].clone();
    let Message::Response { reply, .. } = &mut large.message else {
        unreachable!()
    };
    let Reply::Captured(previous) = reply else {
        unreachable!()
    };
    *reply = Reply::Captured(capture_support::capture(
        previous.before().clone(),
        480,
        480,
    ));
    frames.push(large);
    for (index, frame) in frames.iter().enumerate() {
        let bytes = frame.encode().unwrap();
        assert_eq!(Frame::decode(&bytes[4..]).unwrap(), *frame);
        fs::write(path.join(format!("{index:02}.bin")), bytes)?;
        fs::write(
            path.join(format!("{index:02}.sha256")),
            frame.fingerprint().unwrap(),
        )?;
    }
    println!("{} canonical Rust vectors written", frames.len());
    Ok(())
}
