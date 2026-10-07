"""Actual maintained CLI in disposable repos; no private semantic engine/provider writes."""
import os
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import shutil
import subprocess
import tempfile
from threading import Thread
import unittest
from urllib.parse import parse_qs, urlsplit

from remote_identity import verify_remote_tag
from stage_source import Refusal

TOOL = os.environ.get("PYTHON_SEMANTIC_RELEASE")
CONFIG = Path(__file__).with_name("python-semantic-release.toml").resolve()


class PythonSemanticReleaseCandidate(unittest.TestCase):
    def setUp(self):
        self.assertTrue(TOOL, "Set PYTHON_SEMANTIC_RELEASE to hash-installed upstream CLI")
        self.root = Path(tempfile.mkdtemp(prefix="sdk-psr-owned-"))
        self.addCleanup(shutil.rmtree, self.root)
        self.env = {
            "PATH": os.environ["PATH"], "HOME": str(self.root),
            "GIT_CONFIG_NOSYSTEM": "1", "GIT_CONFIG_GLOBAL": "/dev/null",
            "GIT_AUTHOR_NAME": "Owned Fixture", "GIT_COMMITTER_NAME": "Owned Fixture",
            "GIT_AUTHOR_EMAIL": "fixture@example.invalid", "GIT_COMMITTER_EMAIL": "fixture@example.invalid",
            "GIT_AUTHOR_DATE": "2026-01-01T00:00:00Z", "GIT_COMMITTER_DATE": "2026-01-01T00:00:00Z",
        }
        self.remote = self.root / "remote.git"
        self.command(self.root, "git", "init", "--bare", "--initial-branch=main", str(self.remote))
        self.repo = self.root / "source"
        self.command(self.root, "git", "clone", str(self.remote), str(self.repo))
        (self.repo / "releaserc.toml").write_text(CONFIG.read_text())
        self.git("add", "releaserc.toml")
        self.commit("chore(REM-50): fixture base")
        self.git("push", "origin", "main")
        self.git("remote", "set-url", "origin", "https://github.com/fixture/never-contact.git")
        self.git("config", f"url.{self.remote.as_uri()}.insteadOf", "https://github.com/fixture/never-contact.git")

    def command(self, cwd, *args, allowed=(0,)):
        result = subprocess.run(args, cwd=cwd, env=self.env, capture_output=True, text=True, timeout=30)
        self.assertIn(result.returncode, allowed, (args, result.stdout, result.stderr))
        return result

    def git(self, *args, repo=None):
        return self.command(repo or self.repo, "git", *args).stdout.strip()

    def commit(self, message, repo=None):
        self.git("commit", "--allow-empty", "-m", message, repo=repo)

    def baseline(self, tag="v0.4.0"):
        self.git("tag", tag)
        self.git("push", "origin", tag)

    def version(self, repo=None, push=False):
        return self.command(repo or self.repo, TOOL, "-c", "releaserc.toml", "version",
                        "--no-commit", "--no-changelog", "--skip-build",
                        "--push" if push else "--no-push", "--no-vcs-release", allowed=(0, 1))

    def test_equal_timestamp_tagged_history_and_no_partial_tags(self):
        self.baseline()
        self.commit("feat(REM-50): feature")
        self.assertEqual(self.version(push=True).returncode, 0)
        self.assertEqual(self.git("rev-parse", "v0.5.0^{commit}"), self.git("rev-parse", "HEAD"))
        self.commit("fix(REM-50): repair")
        self.assertEqual(self.version(push=True).returncode, 0)
        self.assertEqual(self.git("rev-parse", "v0.5.1^{commit}"), self.git("rev-parse", "HEAD"))
        self.assertEqual(self.git("tag").splitlines(), ["v0.4.0", "v0.5.0", "v0.5.1"])
        self.assertEqual(len(set(self.git("log", "--format=%ct").splitlines())), 1)
        self.assertEqual(self.git("status", "--porcelain"), "")

    def test_stable_breaking_change_uses_upstream_major(self):
        self.baseline("v1.2.3")
        self.commit("fix(REM-50)!: API changed")
        sha = self.git("rev-parse", "HEAD")
        self.assertEqual(self.version(push=True).returncode, 0)
        self.assertEqual(verify_remote_tag(self.repo, "v2.0.0", sha)["source_sha"], sha)
        self.assertEqual(self.git("rev-parse", "HEAD"), sha)

    def test_initial_docs_raw_cli_requires_upstream_path_gate(self):
        # Remove the baseline chore from release consideration: docs-only first history.
        self.git("checkout", "--orphan", "docs-first")
        self.git("commit", "-m", "docs(REM-50)!: initial documentation\n\nBREAKING CHANGE: docs")
        self.git("branch", "-D", "main")
        self.git("branch", "-m", "main")
        sha = self.git("rev-parse", "HEAD")
        self.assertEqual(self.version().returncode, 0)
        # Observed upstream limitation: without a baseline it emits v0.0.0.
        # Production must skip this command entirely for docs-only events via
        # the qualified upstream path gate, and review first-release policy.
        self.assertEqual(self.git("tag"), "v0.0.0")
        self.assertEqual(self.git("rev-parse", "v0.0.0^{commit}"), sha)
        self.assertEqual(self.git("rev-parse", "HEAD"), sha)

    def test_hostile_body_is_not_a_title_or_command(self):
        self.baseline()
        self.commit("docs(REM-50): examples\n\nfeat(REM-50)!: copied example\nBREAKING CHANGE: copied\n$(touch OWNED_COMMAND_EXECUTED)\n`touch OWNED_BACKTICK_EXECUTED`")
        before = self.git("show-ref", "--tags")
        self.assertEqual(self.version().returncode, 0)
        self.assertEqual(self.git("show-ref", "--tags"), before)
        self.assertFalse((self.repo / "OWNED_COMMAND_EXECUTED").exists())
        self.assertFalse((self.repo / "OWNED_BACKTICK_EXECUTED").exists())

    def test_failed_remote_push_local_tag_and_replay_are_not_build_authority(self):
        self.baseline()
        other = self.root / "other"
        self.command(self.root, "git", "clone", str(self.remote), str(other))
        (other / "releaserc.toml").write_text(CONFIG.read_text())
        self.git("remote", "set-url", "origin", "https://github.com/fixture/never-contact.git", repo=other)
        self.git("config", f"url.{self.remote.as_uri()}.insteadOf", "https://github.com/fixture/never-contact.git", repo=other)
        self.commit("fix(REM-50): winner")
        winner = self.git("rev-parse", "HEAD")
        self.commit("fix(REM-50): loser", repo=other)
        loser = self.git("rev-parse", "HEAD", repo=other)
        self.assertNotEqual(winner, loser)
        self.assertEqual(self.version(push=True).returncode, 0)
        self.assertEqual(self.version(repo=other, push=True).returncode, 1)
        self.assertEqual(self.git("rev-parse", "v0.4.1^{commit}", repo=other), loser)
        self.assertEqual(self.version(repo=other, push=True).returncode, 0)
        with self.assertRaises(Refusal):
            verify_remote_tag(other, "v0.4.1", loser)
        self.assertEqual(verify_remote_tag(self.repo, "v0.4.1", winner)["source_sha"], winner)
        self.assertEqual(self.git("rev-parse", "v0.4.1^{commit}", repo=other), loser)

    def test_absent_or_unreachable_remote_refuses_local_success(self):
        self.baseline()
        self.commit("fix(REM-50): local only")
        sha = self.git("rev-parse", "HEAD")
        self.assertEqual(self.version().returncode, 0)
        with self.assertRaises(Refusal):
            verify_remote_tag(self.repo, "v0.4.1", sha)
        self.git("remote", "set-url", "origin", str(self.root / "missing.git"))
        with self.assertRaises(Refusal):
            verify_remote_tag(self.repo, "v0.4.1", sha)

    def test_annotated_remote_identity_does_not_rewrite_conflicting_local_ref(self):
        sha = self.git("rev-parse", "HEAD")
        self.git("tag", "-a", "v0.4.0", "-m", "Owned annotated tag")
        self.git("push", "origin", "v0.4.0")
        object_sha = self.git("rev-parse", "v0.4.0")
        self.commit("fix(REM-50): local divergent source")
        local_sha = self.git("rev-parse", "HEAD")
        self.git("tag", "-f", "v0.4.0")
        observed = verify_remote_tag(self.repo, "v0.4.0", sha)
        self.assertEqual(observed["remote_tag_object"], object_sha)
        self.assertEqual(self.git("rev-parse", "v0.4.0"), local_sha)


class PythonPublisherCandidate(PythonSemanticReleaseCandidate):
    """Real upstream publish command against a loopback-only modeled API.

    Inherit repository setup, but not the version tests (see load_tests below).
    Fixture HTTP is explicitly enabled only in the temporary configuration.
    """
    def setUp(self):
        super().setUp()
        self.baseline()
        self.events = []
        self.assets = {}
        self.release_exists = True
        self.fail_archive = False
        self.env["GITHUB_REPOSITORY"] = "fixture/never-contact"
        fixture = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_):
                pass

            def reply(self, status, payload):
                data = json.dumps(payload).encode()
                self.send_response(status)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)

            def do_GET(self):
                fixture.events.append(("GET", self.path))
                if self.path == "/api/v3/repos/fixture/never-contact/releases/tags/v0.4.0":
                    self.reply(200 if fixture.release_exists else 404, {"id": 81})
                elif self.path == "/api/v3/repos/fixture/never-contact/releases/81":
                    self.reply(200, {"id": 81, "draft": True, "upload_url": fixture.origin + "/uploads{?name,label}"})
                else:
                    self.reply(400, {"message": "Unexpected fixture endpoint"})

            def do_POST(self):
                fixture.events.append(("POST", self.path))
                data = self.rfile.read(int(self.headers.get("Content-Length", 0)))
                path = urlsplit(self.path)
                name = parse_qs(path.query).get("name", [""])[0]
                if path.path != "/uploads":
                    self.reply(400, {"message": "Unexpected fixture endpoint"})
                elif fixture.fail_archive and name == "sdk.tar":
                    self.reply(400, {"message": "Owned upload failure"})
                elif name in fixture.assets:
                    self.reply(422, {"message": "Owned duplicate asset"})
                else:
                    fixture.assets[name] = data
                    self.reply(201, {"id": 90 + len(fixture.assets), "name": name})

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        self.addCleanup(server.server_close)
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(thread.join, 3)
        self.addCleanup(server.shutdown)
        self.origin = f"http://127.0.0.1:{server.server_port}"
        config = CONFIG.read_text() + f'\ndomain = "{self.origin}"\napi_domain = "{self.origin}"\ninsecure = true\ntoken = "owned-fixture-not-a-credential"\n[semantic_release.publish]\ndist_glob_patterns = ["dist/*"]\n'
        (self.repo / "releaserc.toml").write_text(config)
        (self.repo / "dist").mkdir()
        (self.repo / "dist" / "manifest.json").write_text('{"fixture":"owned"}')
        (self.repo / "dist" / "sdk.tar").write_bytes(b"Owned SDK bytes")

    def publish(self):
        return self.command(self.repo, TOOL, "-c", "releaserc.toml", "publish", "--tag", "v0.4.0", allowed=(0, 1))

    def test_missing_release_success_is_not_publication(self):
        self.release_exists = False
        self.assertEqual(self.publish().returncode, 0)
        self.assertEqual(self.assets, {})
        self.assertTrue(all(method == "GET" for method, _ in self.events))

    def test_missing_files_success_is_not_completeness(self):
        for path in (self.repo / "dist").iterdir():
            path.unlink()
        self.assertEqual(self.publish().returncode, 0)
        self.assertEqual(self.assets, {})

    def test_partial_retry_fills_missing_but_reports_duplicate_error(self):
        self.fail_archive = True
        self.assertEqual(self.publish().returncode, 1)
        self.assertEqual(list(self.assets), ["manifest.json"])
        retained = self.assets["manifest.json"]
        self.fail_archive = False
        self.assertEqual(self.publish().returncode, 1)
        self.assertEqual(self.assets, {"manifest.json": retained, "sdk.tar": b"Owned SDK bytes"})
        self.assertEqual(self.publish().returncode, 1)
        self.assertFalse(any(method in ("DELETE", "PATCH") for method, _ in self.events))
        self.assertTrue(all(method == "GET" or urlsplit(path).path == "/uploads" for method, path in self.events))


def load_tests(loader, tests, pattern):
    # The publisher class reuses setup/helpers, not the superclass's test cases.
    result = loader.loadTestsFromTestCase(PythonSemanticReleaseCandidate)
    for name in PythonPublisherCandidate.__dict__:
        if name.startswith("test_"):
            result.addTest(PythonPublisherCandidate(name))
    return result


if __name__ == "__main__":
    unittest.main()
