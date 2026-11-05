# Publish only a listening endpoint

Buddy runtime CI observed ConnectionRefused after sdk.sock appeared. Both the
owned protocol fixture and native endpoint exposed the final path immediately
after bind, before permission restriction and listen. File existence therefore
did not establish connection readiness. Retrying an established connection or
weakening socket identity checks is not an acceptable workaround.

Bind through the retained activation directory to a private staging socket name.
Refuse either occupied staging or final path. Restrict the pinned socket inode to
0600, start listening, verify the named staged inode still matches, then atomically
rename it to sdk.sock with Linux RENAME_NOREPLACE. Unsupported publication must
refuse without fallback, overwrite or unlink of an unrelated path. Cleanup may
remove only either exact name still identifying the owned inode. Final socket
appearance is now a listening publication boundary; Ready still requires the
authenticated protocol handshake and grants no native Reader/write capability.

Exercise pre-listen refusal, occupied final file/symlink preservation, changed
staged inode refusal, successful immediate connection after publication, final
permissions/identity and existing Rust/Qt exchange. Mirror publication ordering
in Buddy's owned Python peer, with an explicit delayed-staging regression. Keep
client transport's one-connection, deadline, authentication and no-replay rules.
Rebuild and identify the native adapter/profile before any later native run.
