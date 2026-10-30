use remarkable_open_sdk::{OperationId, PageKey, Uuid};
#[test]
fn canonical_identity_round_trips() {
    let text = "01234567-89ab-cdef-0123-456789abcdef";
    let id = Uuid::parse(text).unwrap();
    assert_eq!(id.to_string(), text);
    assert_eq!(
        id.as_bytes(),
        &[
            1, 35, 69, 103, 137, 171, 205, 239, 1, 35, 69, 103, 137, 171, 205, 239
        ]
    );
    let key = PageKey {
        document: id,
        page: id,
    };
    assert_eq!(key.document, OperationId(id).0);
}
#[test]
fn malformed_or_nil_identity_refuses() {
    for text in [
        "",
        "00000000-0000-0000-0000-000000000000",
        "01234567-89AB-cdef-0123-456789abcdef",
        "0123456789ab-cdef-0123-456789abcdef",
        "01234567-89ab-cdef-0123-456789abcdeg",
        "01234567-89ab-cdef-0123-456789abcdef\n",
        "😀1234567-89ab-cdef-0123-456789abcdef",
    ] {
        assert_eq!(Uuid::parse(text), None, "{text:?}");
    }
}
