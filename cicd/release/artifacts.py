"""Shared utility subset from Buddy925f70af; no calculator or release coordinator."""
import hashlib
import os
import stat
import subprocess

def git(repo, *args):
    return subprocess.check_output(["git", *args], cwd=repo, text=True, encoding="utf-8").strip()

def sha256(path):
    if not stat.S_ISREG(path.lstat().st_mode):
        raise ValueError("Expected regular package file")
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()

def output(**values):
    import uuid
    text = ""
    for name, value in values.items():
        delimiter = uuid.uuid4().hex
        text += f"{name}<<{delimiter}\n{value}\n{delimiter}\n"
    print(text, end="")
    if destination := os.environ.get("GITHUB_OUTPUT"):
        with open(destination, "a", encoding="utf-8") as stream:
            stream.write(text)
