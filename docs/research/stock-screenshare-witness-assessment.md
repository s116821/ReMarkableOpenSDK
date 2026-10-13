# Stock Screen Share as a source witness

Assessed October 12, 2026. **Do not select this route as a source5 witness
replacement.** This is a bounded source assessment, not a
tablet test or a finding that Screen Share fails on firmware 3.28.0.172.

Chronology correction: the 57ee result below describes its selected historical
footer policy. The later, separately assessed
[stable-body-idle contract](https://github.com/s116821/ReMarkableBuddiesDocs/blob/024a4db4992e461f71c7f82a098145702aa465e3/openspec/changes/qualify-open-pdf-capture/source5-stable-source-witness-contract.md)
explicitly removes that ordinal witness for its own closed selection, retains
two distinct BODY observations and full idle/context checks, and disclaims native
current-page authority. Its local implementation is accepted; native qualification
is unfinished. The original assessment should not be read as making footer
capture mandatory for that later variant. Neither policy changes here.

## Question and distinction

Can the stock presentation stream independently establish the displayed source
page, avoiding the transient-footer capture dependency? The
[source5 result](https://github.com/s116821/ReMarkableBuddiesDocs/blob/20d56a28f713637370c4d9e7c91ad7e8bcfa54df/openspec/changes/qualify-open-pdf-capture/source5-runtime-preparation-result.md)
records two 57ee frames without the Page6of8 footer and no creation permission.
Its selected gate required the source witness before native creation.

This candidate would consume an existing stock presentation service from the
host. It would not transfer or activate a native payload, restart xochitl, or
read its process memory. That makes the method technically distinct from held
8112 transfer/activation. No device connection, account/configuration read,
installer, input, or service change was performed for this assessment.

## What the sources establish

The current [manufacturer instructions](https://support.remarkable.com/articles/Knowledge/Screen-Share)
require cloud pairing and starting presentation from the document's sharing
menu. Web presentation requires firmware 3.27 or newer. Screen Share presents
one document and deliberately excludes the My files home screen. This is a
document presentation surface, not a documented mirror of the complete tablet
UI. The instructions do not promise inclusion of the transient page footer.
They describe session start/stop; they do not establish a supported automatic
start path or require restarting presentation for every page.

rMview's default branch and latest release resolved to
`8679c7fb9cd536ff1ad906734f72d06e40667942` (v3.1.4). Its pinned
[ScreenShare backend](https://github.com/bordaigorl/rmview/blob/8679c7fb9cd536ff1ad906734f72d06e40667942/src/rmview/screenstream/screenshare.py)
requires no tablet dependency installation. For newer firmware it reads an
account identity from the device token in configuration, receives a broadcast
challenge, and connects to the stock TLS/VNC endpoint. Its input methods are
empty. The [README](https://github.com/bordaigorl/rmview/blob/8679c7fb9cd536ff1ad906734f72d06e40667942/README.md)
requires first enabling Screen Share on the tablet. The separate legacy VNC
backend's installer is not part of this candidate and was not run.

The [frame assembly](https://github.com/bordaigorl/rmview/blob/8679c7fb9cd536ff1ad906734f72d06e40667942/src/rmview/screenstream/common.py)
emits images after RFB rectangle updates. That completion is a client frame
update, not a native page-key, visit-generation, physical-display completion,
or source/order authorization receipt. The client code does not establish
whether the current firmware includes a footer in its server-side presentation.

[Upstream issue 177](https://github.com/bordaigorl/rmview/issues/177)
contains RM2 3.24 distorted/partial-image reports, experimental decoder changes,
and later 3.26 authentication stalls. These are attributed user reports, not
reproduced local defects. Neither those reports nor the broad README support
table verifies success or failure on our exact 3.28.0.172 profile. No repair,
fork, protocol probe, or firmware change was attempted.

## Decision and remaining requirement

Reject this candidate as a source-witness replacement. No supported
start path within the current no-account-change/no-menu-automation workflow has
been established. Even a successful presentation would not, by itself, prove
the historical transient-footer predicate or a general actual active-page/current
visit claim. Distinguishing printed PDF page2 from logical Page6of8 and
binding it to the current source remain necessary; a plausible canvas image or
stored last-opened index is insufficient.

This is a sufficiency decision, not a claim that all stock capture alternatives
are impossible. A one-off developer presentation on an already configured,
explicitly selected device would remain separate from product navigation. It
would first need a supported activation path and actual full-frame evidence of
the required witness on the exact firmware. No account setup, menu automation,
or new screen-sharing infrastructure follows from this assessment. Historical
pairing state is not treated as current device authority.

Main confirmed during this assessment that no current UI observation establishes
Screen Share availability without account setup. That current operator report
has no public transcript link; it is an unverified prerequisite, not evidence
that the feature is absent or disabled.

The narrow unresolved capability is a fresh, current-source display witness
under the existing clocks, or an independently assessed replacement witness
contract. This assessment neither releases held8112 nor authorizes an equivalent
activation elsewhere. A held operation, if indispensable to the selected
implementation, requires review through its original operator/platform path;
renaming or rerouting it is not an alternative method. Existing shutdown,
insertion, preservation, and production qualification gaps remain open.

Source basis: full current [REM-25 issue/comment chronology](https://linear.app/magentumdragon/issue/REM-25),
current [operator handoff](https://mem.ai/8fba6f57-3f5b-5bd2-aa7b-5b283d2b31a7)
(v16 at review), checked-in source5 result, pinned public client source, manufacturer
documentation, and attributed upstream reports. The rejection and capability
comparison are engineering judgments. No new hardware result is claimed.
