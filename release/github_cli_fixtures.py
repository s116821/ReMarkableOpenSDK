"""Pinned upstream CLI on loopback TLS only; modeled server is not GitHub qualification."""
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
from pathlib import Path
import shutil
import ssl
import subprocess
import tempfile
from threading import Lock, Thread
import unittest
from urllib.parse import parse_qs, urlsplit

from stage_source import Refusal
from verify_assets import verify_downloaded_assets

CLI = os.environ.get("SDK_GITHUB_CLI")


class GithubCliCandidate(unittest.TestCase):
    def setUp(self):
        self.assertTrue(CLI, "Set SDK_GITHUB_CLI to verified upstream 2.102.0 binary")
        self.root = Path(tempfile.mkdtemp(prefix="sdk-gh-cli-owned-"))
        self.addCleanup(shutil.rmtree, self.root)
        cert, key = self.root / "cert.pem", self.root / "key.pem"
        subprocess.run(["openssl", "req", "-x509", "-newkey", "rsa:2048", "-nodes",
                        "-keyout", str(key), "-out", str(cert), "-days", "1",
                        "-subj", "/CN=owned-fixture", "-addext", "subjectAltName=IP:127.0.0.1"],
                       check=True, capture_output=True, timeout=15)
        self.events, self.assets = [], {}
        self.release = None
        self.tag_present = True
        self.fail_archive = False
        self.lock = Lock()
        fixture = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_):
                pass

            def handle_request(self):
                path = urlsplit(self.path)
                data = self.rfile.read(int(self.headers.get("Content-Length", 0)))
                with fixture.lock:
                    event = {"method": self.command, "path": path.path}
                    fixture.events.append(event)
                    status, payload = fixture.dispatch(self.command, path, data, event)
                encoded = payload if isinstance(payload, bytes) else json.dumps(payload).encode()
                self.send_response(status)
                self.send_header("Content-Type", "application/octet-stream" if isinstance(payload, bytes) else "application/json")
                self.send_header("Content-Length", str(len(encoded)))
                self.end_headers()
                if self.command != "HEAD":
                    try:
                        self.wfile.write(encoded)
                    except (BrokenPipeError, ConnectionResetError):
                        pass  # Real CLI cancels the losing parallel release lookup.

            do_GET = do_POST = do_PATCH = do_DELETE = do_HEAD = handle_request

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        context.load_cert_chain(cert, key)
        server.socket = context.wrap_socket(server.socket, server_side=True)
        self.addCleanup(server.server_close)
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        self.addCleanup(thread.join, 3)
        self.addCleanup(server.shutdown)
        self.host = f"127.0.0.1:{server.server_port}"
        self.origin = "https://" + self.host
        self.env = {"PATH": os.environ["PATH"], "HOME": str(self.root),
                    "GH_CONFIG_DIR": str(self.root / "config"), "GH_HOST": self.host,
                    "GH_ENTERPRISE_TOKEN": "owned-fixture-not-a-credential",
                    "SSL_CERT_FILE": str(cert), "NO_PROXY": "127.0.0.1",
                    "GH_PROMPT_DISABLED": "1", "GH_NO_UPDATE_NOTIFIER": "1"}
        self.data = b"Owned SDK archive"
        self.expected = {"schema": 1, "git_tag": "v0.4.1", "version": "0.4.1", "source_sha": "a" * 40,
                         "distributions": [{"name": "sdk.tar", "size": len(self.data), "sha256": hashlib.sha256(self.data).hexdigest()}],
                         "compatibility": {"rm1": "unqualified"}, "native_operations": "unsupported"}
        self.manifest, self.archive = self.root / "manifest.json", self.root / "sdk.tar"
        self.manifest.write_text(json.dumps(self.expected))
        self.archive.write_bytes(self.data)

    def snapshot(self):
        return {**self.release, "id": 81, "node_id": "owned-release", "tag_name": "v0.4.1",
                "url": self.origin + "/api/v3/repos/fixture/owned/releases/81",
                "html_url": self.origin + "/fixture/owned/releases/tag/v0.4.1",
                "upload_url": self.origin + "/uploads{?name,label}",
                "assets": [self.asset(name) for name in self.assets]}

    def asset(self, name):
        item = self.assets[name]
        return {"id": item["id"], "node_id": "owned-asset", "name": name, "state": "uploaded",
                "size": len(item["bytes"]), "digest": "sha256:" + hashlib.sha256(item["bytes"]).hexdigest(),
                "url": self.origin + f'/api/v3/repos/fixture/owned/releases/assets/{item["id"]}',
                "browser_download_url": self.origin + f'/download/{item["id"]}'}

    def dispatch(self, method, path, data, event):
        prefix = "/api/v3/repos/fixture/owned/releases"
        if method == "POST" and path.path == "/api/graphql":
            query = json.loads(data)["query"]
            if "RepositoryFindRef" in query:
                ref = {"id": "owned-existing-tag"} if self.tag_present else None
                return 200, {"data": {"repository": {"ref": ref}}}
            if "RepositoryReleaseByTag" in query:
                rel = {"databaseId": 81, "isDraft": True} if self.release and self.release["draft"] else None
                return 200, {"data": {"repository": {"release": rel}}}
        if method in ("GET", "HEAD") and path.path == prefix + "/tags/v0.4.1":
            return (200, self.snapshot()) if self.release and not self.release["draft"] else (404, {})
        if method == "GET" and path.path == prefix + "/81":
            return (200, self.snapshot()) if self.release else (404, {})
        if method == "POST" and path.path == prefix:
            if self.release:
                return 422, {"message": "Owned existing release"}
            self.release = json.loads(data)
            event["draft"] = self.release["draft"]
            return 201, self.snapshot()
        if method == "POST" and path.path == "/uploads":
            name = parse_qs(path.query)["name"][0]
            event["name"] = name
            if self.release and not self.release["draft"]:
                return 422, {"message": "Modeled published immutability"}
            if self.fail_archive and name == "sdk.tar":
                return 400, {"message": "Owned upload failure"}
            if name in self.assets:
                return 422, {"message": "Owned duplicate name"}
            self.assets[name] = {"id": 90 + len(self.assets), "bytes": data}
            return 201, self.asset(name)
        if method == "PATCH" and path.path == prefix + "/81":
            self.release.update(json.loads(data))
            event["draft"] = self.release["draft"]
            event["assets"] = sorted(self.assets)
            return 200, self.snapshot()
        if method == "DELETE" and path.path == prefix + "/81":
            self.release = None
            self.assets.clear()
            return 204, b""
        if method == "GET" and (path.path.startswith("/download/") or path.path.startswith(prefix + "/assets/")):
            asset_id = int(path.path.rsplit("/", 1)[1])
            for item in self.assets.values():
                if item["id"] == asset_id:
                    return 200, item["bytes"]
        return 400, {"message": "Unexpected owned fixture endpoint"}

    def cli(self, *args, expected=0):
        result = subprocess.run([CLI, "release", *args, "--repo", self.host + "/fixture/owned"],
                                cwd=self.root, env=self.env, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, expected, (result.stdout, result.stderr, self.events))
        return result

    def test_missing_tag_refuses_release_creation(self):
        self.tag_present = False
        self.cli("create", "v0.4.1", "--verify-tag", "--draft", "--notes", "Owned fixture", expected=1)
        self.assertIsNone(self.release)
        self.assertTrue(all(e["path"] == "/api/graphql" for e in self.events))

    def test_default_auto_publish_failure_deletes_draft(self):
        self.fail_archive = True
        self.cli("create", "v0.4.1", str(self.manifest), str(self.archive), "--verify-tag", "--notes", "Owned fixture", expected=1)
        self.assertIsNone(self.release)
        self.assertTrue(any(e["method"] == "DELETE" for e in self.events))
        self.assertFalse(any(e["method"] == "PATCH" and e.get("draft") is False for e in self.events))

    def test_existing_draft_is_not_recreated_or_deleted(self):
        self.cli("create", "v0.4.1", "--verify-tag", "--draft", "--notes", "Owned fixture")
        self.cli("upload", "v0.4.1", str(self.manifest))
        retained = self.assets["manifest.json"].copy()
        self.cli("create", "v0.4.1", "--verify-tag", "--draft", "--notes", "Retry", expected=1)
        self.assertEqual(self.assets["manifest.json"], retained)
        self.assertTrue(self.release["draft"])
        self.assertFalse(any(e["method"] == "DELETE" for e in self.events))

    def test_tampered_retained_bytes_refuse_before_finalization(self):
        self.cli("create", "v0.4.1", "--verify-tag", "--draft", "--notes", "Owned fixture")
        self.cli("upload", "v0.4.1", str(self.manifest), str(self.archive))
        self.assets["sdk.tar"]["bytes"] = b"Owned modeled remote corruption"
        downloaded = self.root / "tampered"
        self.cli("download", "v0.4.1", "--dir", str(downloaded))
        with self.assertRaises(Refusal):
            verify_downloaded_assets(self.expected, downloaded)
        # This is a caller/composition gate, not behavior enforced by gh edit.
        self.assertTrue(self.release["draft"])
        self.assertFalse(any(e["method"] == "PATCH" for e in self.events))

    def test_explicit_draft_recovery_download_identity_and_finalization(self):
        self.cli("create", "v0.4.1", "--verify-tag", "--draft", "--notes", "Owned fixture")
        self.cli("upload", "v0.4.1", str(self.manifest))
        self.fail_archive = True
        self.cli("upload", "v0.4.1", str(self.archive), expected=1)
        self.assertTrue(self.release["draft"])
        retained = self.assets["manifest.json"].copy()
        partial = self.root / "partial"
        self.cli("download", "v0.4.1", "--dir", str(partial))
        self.assertFalse(verify_downloaded_assets(self.expected, partial, require_complete=False)["complete"])
        with self.assertRaises(Refusal):
            verify_downloaded_assets(self.expected, partial)
        self.fail_archive = False
        self.cli("upload", "v0.4.1", str(self.archive))
        self.assertEqual(self.assets["manifest.json"], retained)
        complete = self.root / "complete"
        self.cli("download", "v0.4.1", "--dir", str(complete))
        self.assertTrue(verify_downloaded_assets(self.expected, complete)["complete"])
        self.cli("edit", "v0.4.1", "--draft=false")
        published = [e for e in self.events if e["method"] == "PATCH" and e.get("draft") is False]
        self.assertEqual(published[0]["assets"], ["manifest.json", "sdk.tar"])
        self.cli("upload", "v0.4.1", str(self.archive), expected=1)
        self.assertEqual(self.assets["manifest.json"], retained)
        self.assertFalse(any(e["method"] == "DELETE" for e in self.events))


if __name__ == "__main__":
    unittest.main()
