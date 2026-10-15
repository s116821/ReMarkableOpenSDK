# Reader creation versus capture admission

October 15, 2026. Source-only follow-up to the [same-grab result](../experiments/capture-window-reader-structure-result.md), against producer revision `57404bad555de9c41f4fea26d27c3681a9d0251b`.

## What the producer permits

The current capture gate does not require Reader creation. Startup schedules Entry on the application thread (`visual_capture_startup.cpp:53-61`). Entry publishes its initial ready marker before binding the frame observer (`visual_capture_entry.h:97-100`). The retained window must satisfy focus, visibility, activity, engine, root and thread checks (`221-227`). The first admitted queued frame signal publishes the frame marker and disconnects the observer (`258-279`). An already-created request is refused before that marker is admitted.

`imagePermit()` (`294-299`) requires the retained frame/context and binding markers, but no Reader existence predicate. `readerObserver()` (`122-143`) records the census result and returns success when its context survives; it does not require a favorable census stage. The core samples before one grab and after it (`visual_capture_core.h:119-135`). Thus anchor-uniqueness with zero anchors can accompany a normal image result, as the actual experiment demonstrates.

**Conditional conclusion:** if the application presents a loading window before creating its Reader, this producer can admit a capture during that interval. It contains no ordering edge from Reader creation to request admission. The experiment establishes absent anchors in the sampled reachable roots; it does not establish that Reader creation happened later, that this window should contain Reader, or that early admission caused the loading image. The frame callback's queued delivery also does not timestamp the later grab as the particular frame that emitted it.

## Public lifecycle signals and their limits

Qt provides instance-creation observations, but selecting one requires identifying the application's actual creation mechanism and instance:

| Public surface | Established meaning | Remaining limitation |
| --- | --- | --- |
| [Loader asynchronous/Ready](https://doc.qt.io/qt-6.10/qml-qtquick-loader.html#asynchronous-prop) and [loaded](https://doc.qt.io/qt-6.10/qml-qtquick-loader.html#loaded-signal) | Asynchronous instantiation can span frames; Ready means the component has been created and its item is available. Successful initial local loads emit loaded even when statusChanged is absent. | Must identify the exact Loader, its item and containment; creation does not establish document identity or current pixels. |
| [Component status](https://doc.qt.io/qt-6.10/qml-qtqml-component.html#status-prop) | Ready means the component definition can be instantiated. | Does not prove an instance exists. |
| [Component completed](https://doc.qt.io/qt-6.10/qml-qtqml-component.html#completed-signal) | Emitted after instantiation. | Handler execution order is undefined; it does not prove application document loading or rendering finished. |
| [QQmlIncubator Ready](https://doc.qt.io/qt-6.10/qqmlincubator.html#Status-enum) | The incubator's object is fully created. | Requires the exact incubator/object relationship. [setInitialState](https://doc.qt.io/qt-6.10/qqmlincubator.html#setInitialState) is earlier, before complex bindings and component completion. |

These are conditional design candidates, not discovered native application signals. A generic frame signal or a component-definition Ready state cannot replace instance correspondence. Observing a creation event after the fact also needs a justified initial-state strategy; blindly attaching a signal may miss an already-created instance. No property VALUE access or signal attachment to native objects is qualified by this source assessment.

## Smallest missing artifact

Obtain the exact retained application-root or parent creation site that references DocumentView, with provenance binding it to the retained firmware/resource set. It must identify the loading/root branch and the Reader instantiation mechanism: for example the exact Loader source/sourceComponent, active/asynchronous conditions and item containment, or the equivalent factory/incubator path. Follow only a directly referenced helper if that creation condition is delegated there.

The existing reader-internal node 92/node 94 wiring and registration index do not supply this parent edge. The source artifact would establish which lifecycle signal could be relevant; it would still require a separate supported correspondence between that source instance and the actual captured window/root. Neither a new resource sweep nor a new native observation is selected here. No concrete producer repair is justified until that join is supported.

Source basis: current producer source and accepted actual experiment; official Qt 6.10 lifecycle documentation linked above, reviewed with Astra. The early-admission scenario is a source-supported possibility, not an observed application timeline or proven loading cause. Private retained-source lookup reports have no public transcript link. No new tests, build or device operation were performed.
