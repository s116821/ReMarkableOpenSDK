#!/usr/bin/env python3
"""Derived pointers to every SDK Markdown knowledge/spec file; no private data."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "docs/reference/research-evidence-catalog.json"


def records() -> list[dict[str, object]]:
    result = []
    for path in sorted(p for base in (ROOT / "docs", ROOT / "openspec")
                       for p in base.rglob("*.md") if p.is_file()):
        text = path.read_text(encoding="utf-8").replace("\r\n", "\n")
        headings = [{"line": i, "heading": line.lstrip("#").strip()}
                    for i, line in enumerate(text.splitlines(), 1)
                    if line.startswith("#") and line.partition(" ")[0].strip("#") == ""]
        result.append({"path": path.relative_to(ROOT).as_posix(),
                       "title": headings[0]["heading"] if headings else path.stem,
                       "lf_utf8_sha256": hashlib.sha256(text.encode("utf-8")).hexdigest(),
                       "headings": headings})
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--write", action="store_true", help="refresh derived catalog")
    action.add_argument("--check", action="store_true", help="refuse stale/missing catalog (default)")
    action.add_argument("--query", help="literal case-insensitive search of the full source texts")
    args = parser.parse_args()
    entries = records()
    if args.query is not None:
        needle = args.query.casefold()
        if not needle:
            parser.error("query must be nonempty")
        count = 0
        for entry in entries:
            text = (ROOT / str(entry["path"])).read_text(encoding="utf-8")
            if needle in (str(entry["path"]) + "\n" + text).casefold():
                print(f"{entry['path']}: {entry['title']}")
                count += 1
        print(f"{count} matching documents; metadata is a pointer, not evidence qualification")
        return 0
    payload = json.dumps({"schema": "sdk-research-evidence-catalog/1",
                          "scope": ["docs/**/*.md", "openspec/**/*.md"],
                          "derived_only": True, "qualification_inferred": False,
                          "documents": entries}, indent=2, ensure_ascii=False) + "\n"
    if args.write:
        CATALOG.write_text(payload, encoding="utf-8", newline="\n")
    elif not CATALOG.exists() or CATALOG.read_text(encoding="utf-8") != payload:
        print("Research catalog stale/missing; run python tools/research_evidence_catalog.py --write")
        return 1
    print(f"Research catalog current: {len(entries)} Markdown source documents")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
