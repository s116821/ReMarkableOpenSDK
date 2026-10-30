#![cfg(feature = "native-adapter")]
use remarkable_open_sdk::OperationId;
use remarkable_open_sdk::native_adapter::*;
use sha2::{Digest, Sha256};
#[path = "support/resident_capture.rs"]
mod support;

fn request() -> Frame {
    let mut frame =
        HistoricalFrame::decode(include_bytes!("fixtures/resident-history/v2-insertion.bin"))
            .unwrap()
            .frame()
            .clone();
    let Message::Request(Command::InsertAfter(value)) = frame.message else {
        panic!()
    };
    frame.message = Message::Request(Command::CaptureSource(value.source));
    frame
}
fn capture_frame() -> Frame {
    let request = request();
    let Message::Request(Command::CaptureSource(facts)) = &request.message else {
        panic!()
    };
    Frame {
        message: Message::Response {
            request_sha256: request.fingerprint().unwrap(),
            reply: Reply::Captured(support::capture(facts.clone(), 160, 120)),
        },
        ..request
    }
}
fn visual_frame() -> Frame {
    let mut frame = capture_frame();
    let Message::Response {
        reply: Reply::Captured(source),
        ..
    } = frame.message
    else {
        panic!()
    };
    let mut request = request();
    request.message = Message::Request(Command::CaptureVisual);
    frame.message = Message::Response {
        request_sha256: request.fingerprint().unwrap(),
        reply: Reply::VisualCaptured(
            HistoricalVisual::new(
                source.dimensions(),
                source.device_pixel_ratio(),
                source.png().to_vec(),
                source.sha256(),
            )
            .unwrap(),
        ),
    };
    frame
}

#[test]
fn visual_evidence_has_distinct_grammar_and_original_correlation() {
    let visual = visual_frame();
    let bytes = visual.encode().unwrap();
    assert!(bytes.len() > MAX_FRAME);
    assert_eq!(Frame::decode(&bytes[4..]).unwrap(), visual);
    for size in [1, 97, 4096, MAX_FRAME] {
        let mut decoder = Decoder::default();
        let mut answer = None;
        for part in bytes.chunks(size) {
            let (used, result) = decoder.feed(part).unwrap();
            assert_eq!(used, part.len());
            if result.is_some() {
                answer = result;
            }
        }
        assert_eq!(answer, Some(visual.clone()));
    }
    let mut original = request();
    original.message = Message::Request(Command::CaptureVisual);
    assert!(!Command::CaptureVisual.is_effectful());
    for revision in [1u16, 2, 3] {
        for mut old in [original.encode().unwrap(), bytes.clone()] {
            old[8..10].copy_from_slice(&revision.to_be_bytes());
            assert!(HistoricalFrame::decode(&old).is_err());
        }
    }
    let mut scope = original.scope.clone();
    scope.server_nonce = None;
    let mut session = Session::new(scope, original.operation).unwrap();
    let mut ready = session.hello().unwrap();
    ready.message = Message::Ready;
    ready.scope.server_nonce = original.scope.server_nonce;
    session.accept_ready(&ready).unwrap();
    session
        .begin(original.operation, Command::CaptureVisual)
        .unwrap();
    let mut wrong = capture_frame();
    if let Message::Response { request_sha256, .. } = &mut wrong.message {
        *request_sha256 = original.fingerprint().unwrap();
    }
    assert_eq!(session.complete(&wrong), Err(ProtocolError::WrongResponse));
    assert!(matches!(
        session.complete(&visual).unwrap(),
        Reply::VisualCaptured(_)
    ));
    let Message::Request(source) = request().message else {
        panic!()
    };
    let pending = session.begin(original.operation, source).unwrap();
    let mut wrong = visual;
    wrong.sequence = pending.sequence;
    if let Message::Response { request_sha256, .. } = &mut wrong.message {
        *request_sha256 = pending.fingerprint().unwrap();
    }
    assert_eq!(session.complete(&wrong), Err(ProtocolError::WrongResponse));
    assert_eq!(session.close(), LostRequest::NoPendingMutation);
}

#[test]
fn visual_images_require_complete_bounded_decoding() {
    let Message::Response {
        reply: Reply::VisualCaptured(image),
        ..
    } = visual_frame().message
    else {
        panic!()
    };
    assert_eq!(image.procedure(), VISUAL_PROCEDURE);
    for fault in 0..8 {
        let mut png = image.png().to_vec();
        let mut dimensions = image.dimensions();
        let mut dpr = 1.0;
        match fault {
            0 => {
                png.truncate(png.len() - 12);
            }
            1 => png.push(0),
            2 => png[70] ^= 0x80,
            3 => dimensions[0] += 1,
            4 => dpr = f64::NAN,
            5 => dpr = 0.0,
            6 => png.resize(MAX_CAPTURE_PNG + 1, 0),
            _ => png[24] = 0,
        }
        let digest = Sha256::digest(&png).into();
        assert!(
            HistoricalVisual::new(dimensions, dpr, png, digest).is_err(),
            "visual fault {fault}"
        );
    }
}

#[test]
fn visual_attempt_preserves_terminal_insertion_and_selection_ambiguity() {
    let historical =
        HistoricalFrame::decode(include_bytes!("fixtures/resident-history/v2-insertion.bin"))
            .unwrap();
    let Message::Request(Command::InsertAfter(insertion)) = historical.frame().message.clone()
    else {
        panic!()
    };
    let selection = SelectAdjacent {
        expected: insertion.source.clone(),
        target: insertion.source.page().page,
    };
    for command in [
        Command::InsertAfter(insertion),
        Command::SelectAdjacent(selection),
    ] {
        for explicit_unknown in [false, true] {
            let original = request();
            let mut scope = original.scope.clone();
            scope.server_nonce = None;
            let mut session = Session::new(scope, original.operation).unwrap();
            let mut ready = session.hello().unwrap();
            ready.message = Message::Ready;
            ready.scope.server_nonce = original.scope.server_nonce;
            session.accept_ready(&ready).unwrap();
            let effect = session.begin(original.operation, command.clone()).unwrap();
            let expected = LostRequest::Indeterminate {
                operation: effect.operation,
                request_sha256: effect.fingerprint().unwrap(),
            };
            if explicit_unknown {
                let response = Frame {
                    message: Message::Response {
                        request_sha256: effect.fingerprint().unwrap(),
                        reply: Reply::Indeterminate,
                    },
                    ..effect
                };
                assert_eq!(session.complete(&response).unwrap(), Reply::Indeterminate);
            }
            assert_eq!(session.close(), expected);
            assert_eq!(
                session.begin(original.operation, Command::CaptureVisual),
                Err(ProtocolError::Closed)
            );
            assert_eq!(
                session.complete(&visual_frame()),
                Err(ProtocolError::Closed)
            );
            assert_eq!(session.close(), expected);
        }
    }
}
#[test]
fn capture_roundtrip_bounded_fragments_and_original_history() {
    let frame = capture_frame();
    let bytes = frame.encode().unwrap();
    assert!(bytes.len() > MAX_FRAME + 4 && bytes.len() <= MAX_CAPTURE_FRAME + 4);
    assert_eq!(Frame::decode(&bytes[4..]).unwrap(), frame);
    assert_eq!(HistoricalFrame::decode(&bytes).unwrap().encoded(), bytes);
    for size in [1, 3, 4, 97, 4096, MAX_FRAME, bytes.len()] {
        let mut decoder = Decoder::default();
        let mut answer = None;
        for part in bytes.chunks(size) {
            let (used, result) = decoder.feed(part).unwrap();
            assert_eq!(used, part.len());
            if result.is_some() {
                answer = result;
            }
        }
        assert_eq!(answer, Some(frame.clone()));
    }
    for revision in [1u16, 2] {
        for mut old in [request().encode().unwrap(), bytes.clone()] {
            old[8..10].copy_from_slice(&revision.to_be_bytes());
            assert!(HistoricalFrame::decode(&old).is_err());
        }
    }
    for end in [4, 213, MAX_FRAME, bytes.len() - 1] {
        assert!(Frame::decode(&bytes[4..end]).is_err());
        let mut decoder = Decoder::default();
        assert!(decoder.feed(&bytes[..end]).unwrap().1.is_none());
        decoder.close();
        assert_eq!(decoder.feed(&bytes[end..]), Err(ProtocolError::Closed));
    }
    let mut ordinary = request().encode().unwrap();
    ordinary.resize(MAX_FRAME + 5, 0);
    let len = (ordinary.len() - 4) as u32;
    ordinary[..4].copy_from_slice(&len.to_be_bytes());
    assert!(Frame::decode(&ordinary[4..]).is_err());
}
#[test]
fn malformed_images_metadata_and_after_source_are_refused() {
    let Message::Response {
        reply: Reply::Captured(capture),
        ..
    } = capture_frame().message
    else {
        panic!()
    };
    for fault in 0..10 {
        let mut png = capture.png().to_vec();
        let mut dimensions = capture.dimensions();
        let mut dpr = 1.0;
        let mut after = capture.after().clone();
        match fault {
            0 => {
                png.truncate(png.len() - 12);
            } // missing IEND, full pixel stream remains
            1 => png.push(0),
            2 => png[0] = 0,
            3 => png[70] ^= 0x80, // damaged compressed bytes with newly matching SHA
            4 => dimensions[0] += 1,
            5 => dpr = f64::NAN,
            6 => dpr = 0.0,
            7 => png.resize(MAX_CAPTURE_PNG + 1, 0),
            8 => {
                let mut order = after.order().to_vec();
                order.swap(0, 2);
                after = ReaderFacts::new(after.page(), order).unwrap();
            }
            _ => png[24] = 0,
        }
        let digest = Sha256::digest(&png).into();
        assert!(
            HistoricalCapture::new(
                capture.before().clone(),
                after,
                dimensions,
                dpr,
                png,
                digest
            )
            .is_err(),
            "fault {fault}"
        );
    }
    assert!(
        HistoricalCapture::new(
            capture.before().clone(),
            capture.after().clone(),
            capture.dimensions(),
            1.0,
            capture.png().to_vec(),
            [0; 32]
        )
        .is_err()
    );
}
#[test]
fn capture_requires_original_request_and_does_not_settle_effect_loss() {
    let original = request();
    let mut scope = original.scope.clone();
    scope.server_nonce = None;
    let mut session = Session::new(scope, original.operation).unwrap();
    let mut ready = session.hello().unwrap();
    ready.message = Message::Ready;
    ready.scope.server_nonce = original.scope.server_nonce;
    session.accept_ready(&ready).unwrap();
    let Message::Request(command) = original.message else {
        panic!()
    };
    let started = session.begin(original.operation, command).unwrap();
    let mut response = capture_frame();
    response.scope = started.scope;
    response.sequence = started.sequence;
    response.operation = OperationId(ready.scope.connection);
    assert_eq!(
        session.complete(&response),
        Err(ProtocolError::WrongResponse)
    );
    response.operation = original.operation;
    assert!(matches!(
        session.complete(&response).unwrap(),
        Reply::Captured(_)
    ));
    let historical =
        HistoricalFrame::decode(include_bytes!("fixtures/resident-history/v2-insertion.bin"))
            .unwrap();
    let Message::Request(effect) = historical.frame().message.clone() else {
        panic!()
    };
    session.begin(original.operation, effect).unwrap();
    let lost = session.close();
    assert!(matches!(lost, LostRequest::Indeterminate { .. }));
    assert_eq!(session.complete(&response), Err(ProtocolError::Closed));
    assert_eq!(session.close(), lost);
}
