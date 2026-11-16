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
    let captured = capture_support::capture(facts.clone(), 160, 120);
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
    let visual_request = Frame {
        message: Message::Request(Command::CaptureVisual),
        ..frames[18].clone()
    };
    let Reply::Captured(source) = (match &frames[19].message {
        Message::Response { reply, .. } => reply,
        _ => unreachable!(),
    }) else {
        unreachable!()
    };
    let visual = HistoricalVisual::new(
        source.dimensions(),
        source.device_pixel_ratio(),
        source.png().to_vec(),
        source.sha256(),
    )
    .unwrap();
    frames.push(visual_request.clone());
    frames.push(Frame {
        message: Message::Response {
            request_sha256: visual_request.fingerprint().unwrap(),
            reply: Reply::VisualCaptured(visual),
        },
        ..visual_request
    });
    #[cfg(feature = "development-native-pre-enter")]
    for (count, units) in [(3, vec![0, 0xd800, 0x2029]), (0, vec![])] {
        let request = Frame {
            scope: scope.clone(),
            sequence: 1,
            operation: OperationId(id(32)),
            message: Message::Request(Command::CapturePreEnter { offset: 2, count }),
        };
        let request_sha256 = request.fingerprint().unwrap();
        frames.push(request.clone());
        frames.push(Frame {
            message: Message::Response {
                request_sha256,
                reply: Reply::PreEnterText(Some(units)),
            },
            ..request.clone()
        });
        if count == 3 {
            frames.push(Frame {
                message: Message::Response {
                    request_sha256,
                    reply: Reply::PreEnterText(None),
                },
                ..request
            });
        }
    }
    for (index, frame) in frames.iter().enumerate() {
        let bytes = frame.encode().unwrap();
        assert_eq!(Frame::decode(&bytes[4..]).unwrap(), *frame);
        fs::write(path.join(format!("{index:02}.bin")), bytes)?;
        fs::write(
            path.join(format!("{index:02}.sha256")),
            frame.fingerprint().unwrap(),
        )?;
    }
    #[cfg(feature = "development-native-reader-focus")]
    {
        let request = Frame {
            operation: OperationId(id(34)),
            message: Message::Request(Command::ObserveReaderFocus),
            ..frames[2].clone()
        };
        let reply = Frame {
            message: Message::Response {
                request_sha256: request.fingerprint().unwrap(),
                reply: Reply::ReaderFocus(ReaderFocus::InsideScene),
            },
            ..request.clone()
        };
        for (stem, frame) in [
            ("reader-focus-request", request),
            ("reader-focus-reply", reply),
        ] {
            let bytes = frame.encode().unwrap();
            assert_eq!(Frame::decode(&bytes[4..]).unwrap(), frame);
            fs::write(path.join(format!("{stem}.bin")), bytes)?;
            fs::write(
                path.join(format!("{stem}.sha256")),
                frame.fingerprint().unwrap(),
            )?;
        }
    }
    #[cfg(feature = "development-composer-v1")]
    {
        use composer::{
            Attempts, ClearReceipt, Draft, KeyFacts, KeyObservation, Outcome, RangeIdentity,
            RangeState,
        };
        let state = RangeState {
            identity: RangeIdentity {
                facts,
                visit: id(40),
                range: id(41),
            },
            revision: 37,
        };
        let draft = Draft {
            state: state.clone(),
            units: vec![0xfeff, 0xfffe, 0, 0xd800, 0x000a, 0x2029],
        };
        let mutation = Attempts::from_bits(7).unwrap();
        let observation = Attempts::from_bits(5).unwrap();
        for (index, (name, command, result)) in [
            (
                "provision",
                composer::Command::Provision(state.identity.facts.clone()),
                composer::Reply::Provisioned(Outcome::Verified {
                    attempts: mutation,
                    value: state.clone(),
                }),
            ),
            (
                "observe",
                composer::Command::Observe(state.identity.clone()),
                composer::Reply::Observed(Outcome::Verified {
                    attempts: observation,
                    value: draft.clone(),
                }),
            ),
            (
                "clear",
                composer::Command::Clear(draft.clone()),
                composer::Reply::Cleared(Outcome::Verified {
                    attempts: mutation,
                    value: ClearReceipt {
                        identity: state.identity.clone(),
                        before_revision: 37,
                        after_revision: 42,
                    },
                }),
            ),
            (
                "key",
                composer::Command::AwaitKey {
                    identity: state.identity.clone(),
                    keys: vec![0x0100_0004, 0x0100_0005],
                },
                composer::Reply::KeyObserved(Outcome::Verified {
                    attempts: observation,
                    value: KeyObservation {
                        draft,
                        event: id(42),
                        input_serial: 0x0123_4567_89ab_cdef,
                        key: KeyFacts {
                            key: 0x0100_0004,
                            count: 0,
                            modifiers: 0x1234_5678,
                            native_scan_code: 0x8765_4321,
                            native_virtual_key: 0x1122_3344,
                            native_modifiers: 0x5566_7788,
                            auto_repeat: false,
                            spontaneous: true,
                            text: vec![0xfffe, 0xd800, 0x000a],
                        },
                    },
                }),
            ),
        ]
        .into_iter()
        .enumerate()
        {
            let request = Frame {
                scope: scope.clone(),
                sequence: 1,
                operation: OperationId(id(50 + index as u32)),
                message: Message::Request(Command::Composer(command)),
            };
            let response = Frame {
                message: Message::Response {
                    request_sha256: request.fingerprint().unwrap(),
                    reply: Reply::Composer(result),
                },
                ..request.clone()
            };
            for (suffix, frame) in [("request", request), ("reply", response)] {
                let stem = format!("composer-{name}-{suffix}");
                let bytes = frame.encode().unwrap();
                assert_eq!(Frame::decode(&bytes[4..]).unwrap(), frame);
                fs::write(path.join(format!("{stem}.bin")), bytes)?;
                fs::write(
                    path.join(format!("{stem}.sha256")),
                    frame.fingerprint().unwrap(),
                )?;
            }
        }
    }
    println!(
        "{} canonical numbered Rust vectors plus enabled named extensions written",
        frames.len()
    );
    Ok(())
}
