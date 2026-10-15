## Why
The October 15 REM-56 requirement supersedes tag-before-build release ordering. Official SDK tags and releases must follow successful checks, builds and integration of the verified artifacts. Read-only candidate version resolution may occur earlier for build-time embedding; source files never maintain the SDK version.

## What Changes
Compose the maintained qualified release tools into an ordered SDK pipeline with one final release job. Preserve failed-candidate evidence, source ownership, semantic titles, docs exclusions, artifact identity and retry/concurrency safeguards. This SDK-owned change is separate from Buddy/Manager cleanup and does not authorize production tags, repository settings or native operations during development.
