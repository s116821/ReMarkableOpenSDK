# Restrict the preload to its selected process

The first combined native startup reached its ready state, but automatic fixture
navigation refused before dispatch. Its journal contains a home-mount failure
absent from the prior successful worker trial. The cause is not yet established.
One concrete startup defect exists independently of that causal question:
LD_PRELOAD is left in the selected process environment, while the new constructor
terminates any process that does not match the selected PID/start/executable.
An executed helper inheriting that environment can therefore terminate before
its own main. Do not change native password, credentials, mounting or access state
to address a defect in our diagnostic's process scope.

## Owned regression and narrow correction

First demonstrate the behavior using an owned Linux parent, preload DSO and real
fork/exec child. The inherited negative must enter the DSO constructor in the
child and terminate119 before child main. The corrected parent must clear both
LD_PRELOAD and LD_BIND_NOW after its own qualification, then execute the child
without those variables or that preload. Verify the parent's DSO remains loaded
and callable afterward. Never dlclose or unhook as an environment operation.

Use a small original helper that calls unsetenv on both variables and verifies
their absence. It changes only the current process environment; it does not
change systemd global state, disk configuration or the invoking host. In the
actual startup call it only after exact-build installation has consumed
LD_BIND_NOW=1 and verified actual eager bindings. Unexpected failure terminates
the selected candidate and leaves recovery to its independent actor.

The library stays resident after the variable is removed. Its existing eager
bindings are not undone. Keep the original GUI registration and one-request
deadlines, native owner retention, scopes and unsupported capability unchanged.
This addresses exec inheritance, not general post-fork behavior, concurrent
environment mutation, safe unload or native password/home-mount qualification.
Record observed process tasks without claiming that pre-main means no threads.
Explicitly constructed child environments can reintroduce the removed variables;
the correction covers subsequent default inheritance, not every exec caller.

Require independent review, owned child-process results, exact new source/build
and provider closure, and a fresh packet before another tablet attempt. The spent
attempt is never replayed. A subsequent successful UI startup would support the
specific inheritance explanation; owned fixture success alone cannot prove it.

The [owned regression and changed composition build](../../../docs/experiments/insertion-child-environment-owned-result.md)
pass six actual host exec scenarios/126 checks, plus separate ARM helper and
combined compile/link-only checks. Independent source/raw-output review accepted
the helper and placement. This closes owned controls, not the fresh native run.

Source basis: current original startup/environment handling, actual failed
navigation receipt, scoped candidate/prior journals and an explicit causal
hypothesis. No credential or access-control modification is proposed.
