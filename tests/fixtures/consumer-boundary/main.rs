use remarkable_open_sdk::{
    OperationId, PageKey, Uuid,
    navigation::{LogicalDirection, gesture::*},
};

fn main() {
    let id = Uuid::parse("00000001-1111-1111-1111-111111111111").unwrap();
    assert_eq!(Uuid::parse(&id.to_string()), Some(id));
    let page = PageKey {
        document: id,
        page: id,
    };
    assert_eq!(page.page.as_bytes(), OperationId(id).0.as_bytes());
    let plan = plan_gesture(
        TabletModel::Remarkable2,
        Orientation::Portrait,
        LogicalDirection::Next,
    )
    .unwrap();
    assert_eq!(plan.start(), (700, 512));
    assert_eq!(plan.movements().last(), Some(&(100, 512)));
    assert_eq!(
        plan_gesture(
            TabletModel::Remarkable1,
            Orientation::Portrait,
            LogicalDirection::Next
        ),
        Err(UnsupportedGesture::UnimplementedPair)
    );
    println!("ordinary-consumer: identity and pure geometry; no SDK features or native dispatch");
}
