//! Historical inspection is independent of strict live protocol admission.
#![cfg(feature = "native-adapter")]
use remarkable_open_sdk::native_adapter::*;

const REQUEST: &[u8] = include_bytes!("fixtures/resident-history/v1-insertion.bin");
const RESPONSE: &[u8] = include_bytes!("fixtures/resident-history/v1-insertion-observed.bin");

fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

#[test]
fn retained_v1_preserves_original_bytes_and_correlated_digest() {
    let request = HistoricalFrame::decode(REQUEST).unwrap();
    let response = HistoricalFrame::decode(RESPONSE).unwrap();
    assert_eq!(request.wire_revision(), 1);
    assert_eq!(response.wire_revision(), 1);
    assert_eq!(request.encoded(), REQUEST);
    assert_eq!(response.encoded(), RESPONSE);
    assert_eq!(
        hex(&request.fingerprint()),
        "ba17908eb61a97542847ca09df1d6854b552296d4b77f65ee8de496b3a7ffea7"
    );
    assert_eq!(
        hex(&response.fingerprint()),
        "d6c1ed7391a1a1c8fffb34d0d347669cc4bcaaf0b1d91db617966d443e2753f4"
    );
    assert_eq!(request.frame().scope, response.frame().scope);
    assert_eq!(request.frame().sequence, response.frame().sequence);
    assert_eq!(request.frame().operation, response.frame().operation);
    let Message::Request(Command::InsertAfter(insertion)) = &request.frame().message else {
        panic!("retained insertion")
    };
    let Message::Response {
        request_sha256,
        reply: Reply::InsertionObserved {
            target,
            after_order,
        },
    } = &response.frame().message
    else {
        panic!("retained insertion reply")
    };
    assert_eq!(*request_sha256, request.fingerprint());
    assert_eq!(target.document, insertion.source.page().document);
    assert_eq!(target.page, insertion.target);
    assert_eq!(*after_order, insertion.expected_after_order());
    assert_ne!(
        request.frame().fingerprint().unwrap(),
        request.fingerprint()
    );
    for bytes in [REQUEST, RESPONSE] {
        assert_eq!(
            Frame::decode(&bytes[4..]),
            Err(ProtocolError::UnsupportedVersion)
        );
    }
}

#[test]
fn current_revision_is_retained_without_conversion() {
    for bytes in [REQUEST, RESPONSE] {
        let current = HistoricalFrame::decode(bytes)
            .unwrap()
            .frame()
            .encode()
            .unwrap();
        let history = HistoricalFrame::decode(&current).unwrap();
        assert_eq!(history.wire_revision(), PROTOCOL_REVISION);
        assert_eq!(history.encoded(), current);
        assert_eq!(
            history.fingerprint(),
            history.frame().fingerprint().unwrap()
        );
        assert_eq!(Frame::decode(&current[4..]).unwrap(), *history.frame());
    }
}

#[test]
fn old_revision_cannot_smuggle_selection_tags() {
    let mut frame = HistoricalFrame::decode(REQUEST).unwrap().frame().clone();
    let Message::Request(Command::InsertAfter(insertion)) = frame.message.clone() else {
        panic!()
    };
    let selection = SelectAdjacent {
        expected: insertion.source.clone(),
        target: insertion.source.order()[2],
    };
    for message in [
        Message::Request(Command::SelectAdjacent(selection.clone())),
        Message::Request(Command::ReconcileSelection(selection)),
        Message::Response {
            request_sha256: [9; 32],
            reply: Reply::TargetObserved(insertion.source.clone()),
        },
        Message::Response {
            request_sha256: [9; 32],
            reply: Reply::SelectionReconciled {
                current: insertion.source,
                target_present: true,
            },
        },
    ] {
        frame.message = message;
        let mut bytes = frame.encode().unwrap();
        assert!(HistoricalFrame::decode(&bytes).is_ok());
        bytes[8..10].copy_from_slice(&1u16.to_be_bytes());
        assert!(HistoricalFrame::decode(&bytes).is_err());
    }
}

#[test]
fn malformed_or_unknown_history_refuses_without_truncation() {
    for bytes in [REQUEST, RESPONSE] {
        for size in 0..bytes.len() {
            assert!(HistoricalFrame::decode(&bytes[..size]).is_err());
        }
        let mut trailing = bytes.to_vec();
        trailing.push(0);
        assert!(HistoricalFrame::decode(&trailing).is_err());
        let length = (trailing.len() - 4) as u32;
        trailing[..4].copy_from_slice(&length.to_be_bytes());
        assert!(HistoricalFrame::decode(&trailing).is_err());
        for revision in [0u16, 3, u16::MAX] {
            let mut unsupported = bytes.to_vec();
            unsupported[8..10].copy_from_slice(&revision.to_be_bytes());
            assert_eq!(
                HistoricalFrame::decode(&unsupported),
                Err(ProtocolError::UnsupportedVersion)
            );
        }
        let mut wrong_magic = bytes.to_vec();
        wrong_magic[4] = 0;
        assert!(HistoricalFrame::decode(&wrong_magic).is_err());
    }
    assert_eq!(
        HistoricalFrame::decode(&vec![0; MAX_FRAME + 5]),
        Err(ProtocolError::TooLarge)
    );
}
