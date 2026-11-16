//! Composer correspondence and loss behavior. Executed by the hosted CI suite.
#![cfg(feature = "native-adapter")]
use remarkable_open_sdk::native_adapter::*;
use remarkable_open_sdk::{OperationId, PageKey, Uuid};

fn id(n: u32) -> Uuid {
    Uuid::parse(&format!("00000000-0000-0000-0000-{n:012x}")).unwrap()
}

fn session() -> Session {
    let mut session = Session::new(
        ConnectionIdentity {
            runtime: RuntimeIdentity {
                profile_sha256: [1; 32],
                executable_sha256: [2; 32],
                adapter_sha256: [3; 32],
                boot: id(1),
                pid: 17,
                start_ticks: 91,
                activation: id(2),
            },
            connection: id(3),
            server_nonce: None,
        },
        OperationId(id(4)),
    )
    .unwrap();
    let mut ready = session.hello().unwrap();
    ready.scope.server_nonce = Some(id(5));
    ready.message = Message::Ready;
    session.accept_ready(&ready).unwrap();
    session
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

fn response(request: &Frame, reply: Reply) -> Frame {
    Frame {
        message: Message::Response {
            request_sha256: request.fingerprint().unwrap(),
            reply,
        },
        ..request.clone()
    }
}

#[cfg(not(feature = "development-composer-v1"))]
#[test]
fn ordinary_decoder_refuses_composer_requests_and_replies() {
    let request = session()
        .begin(OperationId(id(20)), Command::CaptureSource(facts()))
        .unwrap();
    let mut bytes = request.encode().unwrap();
    // Substitute only the request kind, retaining a valid Facts payload.
    let kind = bytes.len() - (32 + 2 + 16 * facts().order().len()) - 1;
    for tag in 12..=15 {
        bytes[kind] = tag;
        assert_eq!(Frame::decode(&bytes[4..]), Err(ProtocolError::Malformed));
    }
    let mut bytes = response(&request, Reply::Rejected(Rejection::Unsupported))
        .encode()
        .unwrap();
    let kind = bytes.len() - 2;
    for tag in 14..=17 {
        bytes[kind] = tag;
        assert_eq!(Frame::decode(&bytes[4..]), Err(ProtocolError::Malformed));
    }
}

#[cfg(feature = "development-composer-v1")]
mod enabled {
    use super::*;
    use composer::{
        Attempts, ClearReceipt, Draft, KeyFacts, KeyObservation, Outcome, RangeIdentity, RangeState,
    };

    fn attempts(bits: u8) -> Attempts {
        Attempts::from_bits(bits).unwrap()
    }

    fn draft() -> Draft {
        Draft {
            state: RangeState {
                identity: RangeIdentity {
                    facts: facts(),
                    visit: id(30),
                    range: id(31),
                },
                revision: 7,
            },
            units: vec![0xfeff, 0xfffe, 0, 0xd800, 10, 0x2029],
        }
    }

    fn key() -> KeyObservation {
        KeyObservation {
            draft: draft(),
            event: id(32),
            input_serial: 19,
            key: KeyFacts {
                key: 0x0100_0004,
                count: 0,
                modifiers: u32::MAX,
                native_scan_code: 33,
                native_virtual_key: 44,
                native_modifiers: 55,
                auto_repeat: false,
                spontaneous: true,
                text: vec![0xfeff, 0xd800, 13],
            },
        }
    }

    fn verified<T>(value: T, effectful: bool) -> Outcome<T> {
        Outcome::Verified {
            attempts: attempts(if effectful { 7 } else { 5 }),
            value,
        }
    }

    #[test]
    fn exact_units_and_independent_revisions_survive_roundtrip() {
        let expected = draft();
        assert!(expected.text().is_err());
        let mut valid = expected.clone();
        valid.units = vec![0xfeff, 0xfffe, 10, 0x2029];
        assert_eq!(
            valid.text().unwrap().encode_utf16().collect::<Vec<_>>(),
            valid.units
        );
        for (command, reply) in [
            (
                composer::Command::Provision(facts()),
                composer::Reply::Provisioned(verified(expected.state.clone(), true)),
            ),
            (
                composer::Command::Observe(expected.state.identity.clone()),
                composer::Reply::Observed(verified(expected.clone(), false)),
            ),
            (
                composer::Command::Clear(expected.clone()),
                composer::Reply::Cleared(verified(
                    ClearReceipt {
                        identity: expected.state.identity.clone(),
                        before_revision: 7,
                        after_revision: 42,
                    },
                    true,
                )),
            ),
            (
                composer::Command::AwaitKey {
                    identity: expected.state.identity,
                    keys: vec![0x0100_0004],
                },
                composer::Reply::KeyObserved(verified(key(), false)),
            ),
        ] {
            let mut session = session();
            let request = session
                .begin(OperationId(id(40)), Command::Composer(command))
                .unwrap();
            let bytes = request.encode().unwrap();
            assert_eq!(Frame::decode(&bytes[4..]).unwrap(), request);
            let reply = Reply::Composer(reply);
            let frame = response(&request, reply.clone());
            let bytes = frame.encode().unwrap();
            assert_eq!(Frame::decode(&bytes[4..]).unwrap(), frame);
            assert_eq!(session.complete(&frame).unwrap(), reply);
            assert_eq!(session.close(), LostRequest::NoPendingMutation);
        }
    }

    #[test]
    fn only_contract_status_attempt_combinations_are_decoded() {
        let request = session()
            .begin(
                OperationId(id(40)),
                Command::Composer(composer::Command::Clear(draft())),
            )
            .unwrap();
        for (tag, effectful) in [(14, true), (15, false), (16, true), (17, false)] {
            for status in 1..=3 {
                for bits in 0..=u8::MAX {
                    let frame = response(
                        &request,
                        Reply::Composer(composer::Reply::Cleared(Outcome::Refused {
                            attempts: attempts(0),
                        })),
                    );
                    let mut bytes = frame.encode().unwrap();
                    let tail = bytes.len() - 3;
                    bytes[tail..].copy_from_slice(&[tag, status, bits]);
                    let allowed = (status == 1 && matches!(bits, 0 | 5))
                        || (status == 2
                            && (matches!(bits, 1 | 5) || (effectful && matches!(bits, 3 | 7))));
                    assert_eq!(
                        Frame::decode(&bytes[4..]).is_ok(),
                        allowed,
                        "tag={tag} status={status} bits={bits}"
                    );
                    if allowed {
                        bytes.push(0); // Refused/Unknown cannot smuggle partial capability bytes.
                        let size = (bytes.len() - 4) as u32;
                        bytes[..4].copy_from_slice(&size.to_be_bytes());
                        assert_eq!(Frame::decode(&bytes[4..]), Err(ProtocolError::Malformed));
                    }
                }
            }
        }
    }

    #[test]
    fn unknown_mutation_closes_and_preserves_original_identity() {
        for command in [
            composer::Command::Provision(facts()),
            composer::Command::Clear(draft()),
        ] {
            for bits in [1, 3, 5, 7] {
                let mut session = session();
                let request = session
                    .begin(OperationId(id(40)), Command::Composer(command.clone()))
                    .unwrap();
                let reply = match &command {
                    composer::Command::Provision(_) => {
                        composer::Reply::Provisioned(Outcome::Unknown {
                            attempts: attempts(bits),
                        })
                    }
                    _ => composer::Reply::Cleared(Outcome::Unknown {
                        attempts: attempts(bits),
                    }),
                };
                assert!(
                    session
                        .complete(&response(&request, Reply::Composer(reply)))
                        .is_ok()
                );
                let lost = LostRequest::Indeterminate {
                    operation: request.operation,
                    request_sha256: request.fingerprint().unwrap(),
                };
                assert_eq!(session.close(), lost);
                assert_eq!(session.close(), lost);
                assert_eq!(
                    session.begin(OperationId(id(41)), Command::Composer(command.clone())),
                    Err(ProtocolError::Closed)
                );
            }
        }
    }

    #[test]
    fn wrong_capability_revision_or_result_cannot_consume_pending_clear() {
        let expected = draft();
        let valid = ClearReceipt {
            identity: expected.state.identity.clone(),
            before_revision: 7,
            after_revision: 11,
        };
        let mut foreign_visit = valid.clone();
        foreign_visit.identity.visit = id(77);
        let mut foreign_range = valid.clone();
        foreign_range.identity.range = id(78);
        let mut foreign_facts = valid.clone();
        foreign_facts.identity.facts =
            ReaderFacts::new(facts().page(), vec![id(12), id(11)]).unwrap();
        let mut wrong_revision = valid.clone();
        wrong_revision.before_revision = 6;
        let mut invalid_revision = valid;
        invalid_revision.after_revision = -1;
        let mut replies = vec![
            Reply::Indeterminate,
            Reply::Rejected(Rejection::BeforeDispatch),
            Reply::Rejected(Rejection::ExecutorBeforeMutation),
            Reply::Composer(composer::Reply::Observed(verified(expected.clone(), false))),
            Reply::Composer(composer::Reply::Cleared(Outcome::Verified {
                attempts: attempts(5),
                value: ClearReceipt {
                    identity: expected.state.identity.clone(),
                    before_revision: 7,
                    after_revision: 11,
                },
            })),
        ];
        replies.extend(
            [
                foreign_visit,
                foreign_range,
                foreign_facts,
                wrong_revision,
                invalid_revision,
            ]
            .into_iter()
            .map(|value| Reply::Composer(composer::Reply::Cleared(verified(value, true)))),
        );
        for reply in replies {
            let mut session = session();
            let request = session
                .begin(
                    OperationId(id(40)),
                    Command::Composer(composer::Command::Clear(expected.clone())),
                )
                .unwrap();
            assert_eq!(
                session.complete(&response(&request, reply)),
                Err(ProtocolError::WrongResponse)
            );
            assert_eq!(
                session.close(),
                LostRequest::Indeterminate {
                    operation: request.operation,
                    request_sha256: request.fingerprint().unwrap()
                }
            );
        }
        for reason in [
            Rejection::Unsupported,
            Rejection::UnknownIdentity,
            Rejection::Capacity,
        ] {
            let mut session = session();
            let request = session
                .begin(
                    OperationId(id(40)),
                    Command::Composer(composer::Command::Clear(expected.clone())),
                )
                .unwrap();
            assert!(
                session
                    .complete(&response(&request, Reply::Rejected(reason)))
                    .is_ok()
            );
            assert_eq!(session.close(), LostRequest::NoPendingMutation);
        }
    }

    #[test]
    fn input_selection_and_raw_event_bounds_are_enforced() {
        let identity = draft().state.identity;
        for keys in [
            vec![],
            vec![0],
            vec![-1],
            vec![0x01ff_ffff],
            vec![1, 1],
            vec![1, 2, 3, 4, 5],
        ] {
            assert_eq!(
                session().prepare(
                    OperationId(id(40)),
                    Command::Composer(composer::Command::AwaitKey {
                        identity: identity.clone(),
                        keys,
                    })
                ),
                Err(ProtocolError::Malformed)
            );
        }
        let mut session = session();
        let request = session
            .begin(
                OperationId(id(40)),
                Command::Composer(composer::Command::AwaitKey {
                    identity,
                    keys: vec![0x0100_0005],
                }),
            )
            .unwrap();
        assert_eq!(
            session.complete(&response(
                &request,
                Reply::Composer(composer::Reply::KeyObserved(verified(key(), false)))
            )),
            Err(ProtocolError::WrongResponse)
        );
        for change in 0..4 {
            let mut value = key();
            match change {
                0 => value.input_serial = 0,
                1 => value.key.key = 0x01ff_ffff,
                2 => value.key.text = vec![0; composer::MAX_KEY_TEXT_UNITS + 1],
                _ => value.draft.units = vec![0; composer::MAX_DRAFT_UNITS + 1],
            }
            assert!(
                response(
                    &request,
                    Reply::Composer(composer::Reply::KeyObserved(verified(value, false)))
                )
                .encode()
                .is_err()
            );
        }
        let frame = response(
            &request,
            Reply::Composer(composer::Reply::KeyObserved(verified(key(), false))),
        );
        let mut bytes = frame.encode().unwrap();
        let spontaneous = bytes.len() - key().key.text.len() * 2 - 2 - 1;
        bytes[spontaneous] = 2;
        assert_eq!(Frame::decode(&bytes[4..]), Err(ProtocolError::Malformed));
    }
}
