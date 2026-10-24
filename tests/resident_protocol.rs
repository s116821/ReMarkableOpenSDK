#![cfg(feature = "native-adapter")]

use remarkable_open_sdk::native_adapter::*;
use remarkable_open_sdk::{OperationId, PageKey, Uuid};

fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}
fn facts() -> ReaderFacts {
    ReaderFacts::new(
        PageKey {
            document: id(10),
            page: id(2),
        },
        vec![id(1), id(2), id(3)],
    )
    .unwrap()
}
fn insertion() -> InsertAfter {
    InsertAfter {
        source: facts(),
        target: id(4),
    }
}
fn scope() -> ConnectionIdentity {
    ConnectionIdentity {
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
    }
}
fn session() -> Session {
    let mut value = Session::new(scope(), OperationId(id(23))).unwrap();
    let mut ready = value.hello().unwrap();
    ready.message = Message::Ready;
    ready.scope.server_nonce = Some(id(24));
    value.accept_ready(&ready).unwrap();
    value
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
fn inserted() -> Reply {
    Reply::InsertionObserved {
        target: PageKey {
            document: id(10),
            page: id(4),
        },
        after_order: vec![id(1), id(2), id(4), id(3)],
    }
}

#[test]
fn every_fragment_boundary_and_coalesced_frames() {
    let frame = session()
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    let bytes = frame.encode().unwrap();
    for split in 0..bytes.len() {
        let mut decoder = Decoder::default();
        assert_eq!(decoder.feed(&bytes[..split]).unwrap(), (split, None));
        assert_eq!(
            decoder.feed(&bytes[split..]).unwrap(),
            (bytes.len() - split, Some(frame.clone()))
        );
    }
    let mut decoder = Decoder::default();
    for (n, byte) in bytes.iter().enumerate() {
        let (used, result) = decoder.feed(&[*byte]).unwrap();
        assert_eq!(used, 1);
        assert_eq!(result, (n + 1 == bytes.len()).then(|| frame.clone()));
    }
    let joined = [bytes.clone(), bytes.clone()].concat();
    let (used, first) = decoder.feed(&joined).unwrap();
    assert_eq!((used, first), (bytes.len(), Some(frame.clone())));
    assert_eq!(
        decoder.feed(&joined[used..]).unwrap(),
        (bytes.len(), Some(frame))
    );
}

#[test]
fn every_truncation_fails_decode_and_eof_never_completes() {
    let frame = session()
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    let bytes = frame.encode().unwrap();
    for end in 0..bytes.len() {
        let mut decoder = Decoder::default();
        assert_eq!(decoder.feed(&bytes[..end]).unwrap().1, None);
        decoder.close();
        assert_eq!(decoder.feed(&bytes[end..]), Err(ProtocolError::Closed));
        if end >= 4 {
            assert!(Frame::decode(&bytes[4..end]).is_err());
        }
    }
}

#[test]
fn rejects_lengths_before_allocating_or_consuming_payload() {
    for length in [0, (MAX_FRAME + 1) as u32, u32::MAX] {
        let mut bytes = length.to_be_bytes().to_vec();
        bytes.extend(vec![0; 200_000]);
        let mut decoder = Decoder::default();
        assert!(decoder.feed(&bytes).is_err());
        assert_eq!(decoder.feed(&[]), Err(ProtocolError::Closed));
    }
}

#[test]
fn rejects_unknown_schema_trailing_bytes_nil_ids_and_duplicate_order() {
    let frame = session()
        .begin(OperationId(id(30)), Command::Observe)
        .unwrap();
    let bytes = frame.encode().unwrap();
    let mut future = bytes[4..].to_vec();
    future[5] = 2;
    assert_eq!(
        Frame::decode(&future),
        Err(ProtocolError::UnsupportedVersion)
    );
    let mut unknown = bytes[4..].to_vec();
    *unknown.last_mut().unwrap() = 255;
    assert_eq!(Frame::decode(&unknown), Err(ProtocolError::Malformed));
    let mut trailing = bytes[4..].to_vec();
    trailing.push(0);
    assert_eq!(Frame::decode(&trailing), Err(ProtocolError::Malformed));
    let mut nil_boot = bytes[4..].to_vec();
    nil_boot[102..118].fill(0);
    assert_eq!(
        Frame::decode(&nil_boot),
        Err(ProtocolError::InvalidIdentity)
    );
    assert!(ReaderFacts::new(facts().page(), vec![id(2), id(2)]).is_err());
    assert!(ReaderFacts::new(facts().page(), vec![id(1), id(3)]).is_err());
    let mut bad = scope();
    bad.runtime.start_ticks = 0;
    assert!(Session::new(bad, OperationId(id(23))).is_err());
}

#[test]
fn handshake_rejects_each_changed_runtime_component_and_connection() {
    let original = scope();
    let mut changes = vec![];
    let mut changed = original.clone();
    changed.runtime.profile_sha256[0] ^= 1;
    changes.push(changed);
    let mut changed = original.clone();
    changed.runtime.executable_sha256[0] ^= 1;
    changes.push(changed);
    let mut changed = original.clone();
    changed.runtime.adapter_sha256[0] ^= 1;
    changes.push(changed);
    let mut changed = original.clone();
    changed.runtime.boot = id(99);
    changes.push(changed);
    let mut changed = original.clone();
    changed.runtime.pid += 1;
    changes.push(changed);
    let mut changed = original.clone();
    changed.runtime.start_ticks += 1;
    changes.push(changed);
    let mut changed = original.clone();
    changed.runtime.activation = id(99);
    changes.push(changed);
    let mut changed = original.clone();
    changed.connection = id(99);
    changes.push(changed);
    for mut scope in changes {
        scope.server_nonce = Some(id(24));
        let mut client = Session::new(original.clone(), OperationId(id(23))).unwrap();
        let frame = Frame {
            scope,
            sequence: 0,
            operation: OperationId(id(23)),
            message: Message::Ready,
        };
        assert_eq!(client.accept_ready(&frame), Err(ProtocolError::WrongScope));
        assert_eq!(
            client.begin(OperationId(id(30)), Command::Observe),
            Err(ProtocolError::Closed)
        );
    }
}

#[test]
fn stale_duplicate_wrong_digest_and_wrong_operation_cannot_complete() {
    let mut client = session();
    let first = client.begin(OperationId(id(30)), Command::Observe).unwrap();
    let first_reply = response(&first, Reply::Observed(facts()));
    client.complete(&first_reply).unwrap();
    assert!(client.complete(&first_reply).is_err());
    let second = client
        .begin(OperationId(id(31)), Command::InsertAfter(insertion()))
        .unwrap();
    assert_eq!(
        client.complete(&first_reply),
        Err(ProtocolError::WrongResponse)
    );
    let proper = response(&second, inserted());
    let mut wrong = proper.clone();
    wrong.operation = OperationId(id(30));
    assert_eq!(client.complete(&wrong), Err(ProtocolError::WrongResponse));
    let mut wrong = proper.clone();
    wrong.sequence += 1;
    assert_eq!(client.complete(&wrong), Err(ProtocolError::WrongResponse));
    let mut wrong = proper.clone();
    if let Message::Response { request_sha256, .. } = &mut wrong.message {
        request_sha256[0] ^= 1;
    }
    assert_eq!(client.complete(&wrong), Err(ProtocolError::WrongResponse));
    let mut wrong = proper.clone();
    wrong.scope.connection = id(90);
    assert_eq!(client.complete(&wrong), Err(ProtocolError::WrongScope));
    assert_eq!(
        client.begin(OperationId(id(32)), Command::Observe),
        Err(ProtocolError::Busy)
    );
    assert_eq!(client.complete(&proper).unwrap(), inserted());
}

#[test]
fn insertion_success_requires_exact_target_and_only_permitted_order_change() {
    let mut client = session();
    let request = client
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    for (target, after_order) in [
        (
            PageKey {
                document: id(99),
                page: id(4),
            },
            vec![id(1), id(2), id(4), id(3)],
        ),
        (
            PageKey {
                document: id(10),
                page: id(5),
            },
            vec![id(1), id(2), id(5), id(3)],
        ),
        (
            PageKey {
                document: id(10),
                page: id(4),
            },
            vec![id(1), id(4), id(2), id(3)],
        ),
        (
            PageKey {
                document: id(10),
                page: id(4),
            },
            vec![id(1), id(2), id(4), id(3), id(5)],
        ),
    ] {
        assert_eq!(
            client.complete(&response(
                &request,
                Reply::InsertionObserved {
                    target,
                    after_order
                }
            )),
            Err(ProtocolError::WrongResponse)
        );
    }
    assert_eq!(
        client.complete(&response(&request, Reply::Observed(facts()))),
        Err(ProtocolError::WrongResponse)
    );
    assert!(
        matches!(client.close(), LostRequest::Indeterminate { operation: OperationId(value), .. } if value == id(30))
    );
    assert!(client.complete(&response(&request, inserted())).is_err());
}

#[test]
fn partial_or_zero_write_loss_is_indeterminate_and_read_loss_is_not_mutation() {
    let mut client = session();
    let request = client
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    let expected = LostRequest::Indeterminate {
        operation: request.operation,
        request_sha256: request.fingerprint().unwrap(),
    };
    assert_eq!(client.close(), expected);
    assert_eq!(
        client.close(),
        expected,
        "closing twice must retain unresolved mutation"
    );
    assert_eq!(
        client.begin(OperationId(id(31)), Command::InsertAfter(insertion())),
        Err(ProtocolError::Closed)
    );
    let mut client = session();
    client.begin(OperationId(id(30)), Command::Observe).unwrap();
    assert_eq!(client.close(), LostRequest::NoPendingMutation);
}

#[test]
fn all_reply_variants_roundtrip_without_creating_operational_authority() {
    let request = session()
        .begin(OperationId(id(30)), Command::Observe)
        .unwrap();
    for reply in [
        Reply::Observed(facts()),
        inserted(),
        Reply::Reconciled {
            current: facts(),
            target_present: false,
        },
        Reply::Indeterminate,
        Reply::Rejected(Rejection::Unsupported),
        Reply::Rejected(Rejection::BeforeDispatch),
        Reply::Rejected(Rejection::ExecutorBeforeMutation),
        Reply::Rejected(Rejection::UnknownIdentity),
        Reply::Rejected(Rejection::Capacity),
    ] {
        let frame = response(&request, reply);
        assert_eq!(Frame::decode(&frame.encode().unwrap()[4..]).unwrap(), frame);
    }
}

#[test]
fn rejects_creation_whose_after_order_cannot_be_represented() {
    let order: Vec<_> = (1..=MAX_PAGES as u32).map(id).collect();
    let source = ReaderFacts::new(
        PageKey {
            document: id(9000),
            page: id(2),
        },
        order,
    )
    .unwrap();
    let mut client = session();
    let value = InsertAfter {
        source,
        target: id(8000),
    };
    assert_eq!(
        client.begin(OperationId(id(30)), Command::InsertAfter(value.clone())),
        Err(ProtocolError::InvalidOrder)
    );
    assert!(
        client
            .begin(OperationId(id(30)), Command::Reconcile(value))
            .is_ok()
    );
}

#[test]
fn many_requests_have_monotonic_sequences_and_no_live_history_growth() {
    let mut client = session();
    for n in 1..=10_000 {
        let request = client
            .begin(OperationId(id(n + 100)), Command::Observe)
            .unwrap();
        assert_eq!(request.sequence, u64::from(n));
        client
            .complete(&response(&request, Reply::Observed(facts())))
            .unwrap();
    }
}

#[test]
fn explicit_unknown_mutation_is_sticky_and_cannot_be_retried() {
    let mut client = session();
    let request = client
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    assert_eq!(
        client
            .complete(&response(&request, Reply::Indeterminate))
            .unwrap(),
        Reply::Indeterminate
    );
    let expected = LostRequest::Indeterminate {
        operation: request.operation,
        request_sha256: request.fingerprint().unwrap(),
    };
    assert_eq!(client.close(), expected);
    assert_eq!(client.close(), expected);
    assert_eq!(
        client.begin(request.operation, Command::InsertAfter(insertion())),
        Err(ProtocolError::Closed)
    );
    assert_eq!(
        client.complete(&response(
            &request,
            Reply::Rejected(Rejection::BeforeDispatch)
        )),
        Err(ProtocolError::Closed)
    );
}

#[test]
fn fresh_server_nonce_prevents_old_reply_even_if_caller_reuses_its_nonce() {
    let mut old = session();
    let old_request = old
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    let mut fresh = Session::new(scope(), OperationId(id(23))).unwrap();
    let hello = fresh.hello().unwrap();
    assert_eq!(Frame::decode(&hello.encode().unwrap()[4..]).unwrap(), hello);
    let mut ready = hello;
    ready.message = Message::Ready;
    assert_eq!(ready.encode(), Err(ProtocolError::InvalidIdentity));
    assert_eq!(
        fresh.accept_ready(&ready),
        Err(ProtocolError::InvalidIdentity)
    );
    ready.scope.server_nonce = Some(id(25));
    assert_eq!(Frame::decode(&ready.encode().unwrap()[4..]).unwrap(), ready);
    fresh.accept_ready(&ready).unwrap();
    let request = fresh
        .begin(OperationId(id(30)), Command::InsertAfter(insertion()))
        .unwrap();
    assert_eq!(
        fresh.complete(&response(&old_request, inserted())),
        Err(ProtocolError::WrongScope)
    );
    assert_eq!(
        fresh.complete(&response(&request, inserted())).unwrap(),
        inserted()
    );
    assert_eq!(fresh.close(), LostRequest::NoPendingMutation);
}
