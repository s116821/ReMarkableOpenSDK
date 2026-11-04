"""Remote source identity only: never calculates versions, tags, builds or publishes."""
import re
import subprocess
from pathlib import Path

from stage_source import Refusal


def verify_remote_tag(repo, tag, source_sha, remote="origin"):
    """Observe/fetch the exact remote tag without trusting or changing local tags.

    This verifies identity at the observation boundary. Repository tag protection
    and immutable publication must separately prevent later remote replacement.
    """
    if not re.fullmatch(r"v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)", tag):
        raise Refusal("Remote identity requires an exact stable tag")
    if not re.fullmatch(r"[a-f0-9]{40}", source_sha):
        raise Refusal("Remote identity requires an exact source SHA")
    if not re.fullmatch(r"[a-zA-Z0-9][a-zA-Z0-9_.-]*", remote):
        raise Refusal("Remote identity requires a configured remote name")
    repo = Path(repo).resolve()
    ref = f"refs/tags/{tag}"

    def git(*args):
        try:
            return subprocess.check_output(
                ["git", *args], cwd=repo, stderr=subprocess.PIPE, timeout=30,
                text=True,
            ).strip()
        except (subprocess.SubprocessError, OSError):
            raise Refusal("Remote tag identity could not be verified") from None

    def observe():
        entries = {}
        for line in git("ls-remote", "--exit-code", "--tags", remote, ref, ref + "^{}").splitlines():
            fields = line.split("\t")
            if (len(fields) != 2 or fields[1] not in (ref, ref + "^{}")
                    or not re.fullmatch(r"[a-f0-9]{40}", fields[0]) or fields[1] in entries):
                raise Refusal("Remote tag observation is invalid")
            entries[fields[1]] = fields[0]
        if ref not in entries or entries.get(ref + "^{}", entries[ref]) != source_sha:
            raise Refusal("Remote tag differs from intended source")
        return entries

    before = observe()
    # Download the remote object, never overwrite a conflicting local tag/ref or
    # infer success from FETCH_HEAD left by some other command.
    git("fetch", "--no-tags", "--no-write-fetch-head", remote, ref)
    if git("rev-parse", "--verify", before[ref] + "^{commit}") != source_sha:
        raise Refusal("Fetched remote tag does not identify intended commit")
    if observe() != before:
        raise Refusal("Remote tag changed during verification")
    return {"git_tag": tag, "source_sha": source_sha, "remote_tag_object": before[ref]}
