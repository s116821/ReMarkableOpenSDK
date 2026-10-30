# Owned child-environment regression

October 23, 2026 UTC. Six real Linux host parent/exec-child scenarios passed
**126 checks**. The unchanged parent DSO remained mapped and callable after
clearing inherited injection variables and after child execution. A separate
ARM hard-float helper compile passed; the ARM object was not executed.

These original [owned fixtures](../../scripts/insertion_guard_owned/child_environment/test.sh)
and the [small environment helper](../../scripts/reader_insertion_child_environment.h)
address the [selected-process correction](../../openspec/changes/establish-native-platform-contract/native-insertion-child-environment.md).
They do not prove the cause of the prior [native startup/navigation refusal](insertion-guard-startup-navigation-result.md).

## Actual owned execution

An owned preload constructor checks the expected parent PID and eager-binding
environment before main. It rejects a different child PID with exit119. The
owned parent uses actual kernel fork/exec, captures child output and checks
normal exit status. The child reports success only after reaching its own main
with both variables absent and no owned preload symbol loaded.

| Scenario | Actual child result |
| --- | --- |
| Original environment inherited | Constructor denial119, no child main |
| Parent clears before exec | Child main0 without injection |
| Constructor qualifies, then clears before parent main | Child main0 without injection |
| Repeated clearing of absent variables | Child main0 without injection |
| Explicit custom exec environment reintroduces injection | Constructor denial119, no child main |
| Bare parent and all four variable-presence combinations | Child main0; no parent DSO loaded |

For all five preloaded parent scenarios, the exported cookie resolves to the
same address and returns the same qualified value before and after child
execution. The cases that clear the environment also preserve it across that
change; the inherited negative deliberately leaves the environment intact.
`dl_iterate_phdr` independently checks the exact DSO name
and load base. No dlclose, native hook removal or owner release occurs. The
builder's outer environment remains unchanged. These are real owned host ELF
executions, not mocked exec results or ARM binfmt assumptions.

The first build failed because warning-as-error rejected discarded `write`
return values in test logging. That source/output remains preserved. The second
fixture binds the write return value in its logging code and passes. No native target code is
executed by these fixtures.

## Source, build and review binding

| Input/evidence | SHA-256 |
| --- | --- |
| Helper | `eee2104f1789ef2f7c4902b2c33fb63be49414e931ccc265426dc2cb0abe3f55` |
| Owned preload | `9db51d3138b5e633c604e0fac4360638c1dee3c24dca50d3de4f86965116b8b9` |
| Parent | `e9733a33e3efadcbf236070ad01f7e9539dfc1520cda72c2cf78b59db4bc582e` |
| Child | `0ae2f9c055caf5711ff7c6c79fdb6d036546272b08c1b6a1017aefd5d004be52` |
| Test script | `336e1f73bd6b3f20f8e4b00d25f1fc9f851c311b52539c1824c5b27b30a112f4` |
| Owned R2 receipt | `ff52f866fe2f9a4cfa450e085ee0070f4d87bb86ac68fe32df0cffa9f4eefd25` |
| Combined ARM compile/link-only receipt | `cc74b4b6c99c996245163b1c27552d48e5eb71120699a41933803c3fa38553ff` |
| Independent helper/integration/result review | `d5069b67b5b880f4f0b4f3492fed67a79622ddbb8519d0c231e5d5fc778cbc1b` |

The combined composition compiled/linked all eighteen transitive sources with
the helper and startup `094ef2aa26141286a024833ae2cc138534c027ee733cc9a95eba6821bafabf8e`.
Its dummy-configured DSO `fa225a0df914d6059f65a0448ed560ec55701fbbb70656a4cb3d777ac23f93b2`
was never loaded. A real native artifact must separately bind its own constants,
imports/providers, packet, process and recovery. Unchanged coordinator, predicate,
hook and transport matrices retain their original evidence; they were not rerun.

Independent read review accepted the helper, actual owned sources/raw outputs,
ARM compile classification and startup placement. Qualification and the final
identity check occur before clearing either variable; facilities publication
occurs afterward. Removing variables does not undo already resolved bindings.

## Limits

The helper changes the current process environment only. An explicit custom
environment can reintroduce either variable, as the negative control shows.
Already forked children or separately cached environments are not changed.
No global provider-thread environment quiescence, general post-fork safety,
safe unload, native owner reclamation or observed mount-helper exit is proved.
The prior native startup saw two process tasks; pre-main alone does not imply
the absence of other threads. A fresh changed native run remains necessary.

Source basis: current original sources, saved host exec logs/ELF identities,
separate ARM compilation and independent read review. Private raw receipts and
both test revisions remain local in the investigation continuation directory.
