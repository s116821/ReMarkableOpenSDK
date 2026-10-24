// Entirely owned composition; does not reproduce proprietary UI code.
import Owned.Closed 1.0
Owner {
    id: root
    windowNavigator: ownedNavigator
    windowLayout: ownedLayout
    property bool documentViewActive: documentView.item ? documentView.item.documentLoaded : false
    ClosedLoader {
        id: documentView
        visible: false
        item: ClosedReader {}
    }
}
