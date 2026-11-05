# Caller progress during resident IO

The installed Buddy main thread must observe cancellation and service health
while connecting to the resident endpoint and waiting for a reply. The existing
transport's absolute deadline bounds IO but does not provide this opportunity.

Add `connect_with_progress` and `execute_with_progress`; keep the existing APIs
as no-op-progress conveniences. The callback runs on the caller's thread and
returns `io::Result<()>`. It must be bounded, must not reenter the connection,
and must not dispatch native effects. SDK code cannot interrupt the callback or
the synchronous durable journal; it checks the unchanged deadline after each.

Poll waits use slices of at most 50 milliseconds, also checking progress at IO
boundaries. This is cancellation servicing, not a completion delay or a hard
real-time scheduling guarantee. Readiness, fragments and interrupted system
calls never renew the original deadline. Progress is rechecked after journaling
and authentication immediately before pending admission, and after reading and
authenticating the reply before completing the request. Handshake publication
also requires a final successful progress/deadline check.

A callback failure closes the connection and is reported separately from IO,
deadline and journal failure. Before pending admission, no request bytes have
been sent and a newly prepared effect remains known not dispatched. Once
admitted, failures preserve the original indeterminate operation/digest, even
if a complete response has arrived but has not passed final admission. Prior
uncertainty is never cleared. Closing the socket does not prove native physical
cancellation, reclamation, persistence or permission to replay.

Exercise actual Linux sockets: refusal before connect/dispatch, cancellation
after durable reservation, stalled and fragmented handshake/reply waits,
deadline expiry inside progress, late cancellation with a complete response,
and successful repeated requests. Cover both insertion and selection effects.
This is transport/source evidence, not native or product-flow qualification.
