"""Verify downloaded artifact bytes only; no downloader or release orchestration."""
import hashlib
import json
import os
from pathlib import Path
import re
import stat
from stage_source import Refusal


def _regular_bytes(path, maximum):
    try:
        fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
        with os.fdopen(fd,'rb') as stream:
            before=os.fstat(stream.fileno())
            if not stat.S_ISREG(before.st_mode) or before.st_size>maximum:
                raise Refusal('Asset is not a bounded regular file')
            value=stream.read(maximum+1)
            after=os.fstat(stream.fileno())
            if len(value)>maximum or (before.st_size,before.st_mtime_ns)!=(after.st_size,after.st_mtime_ns):
                raise Refusal('Asset changed or exceeded its declared bound')
            return value
    except OSError as error:raise Refusal('Asset cannot be read safely') from error


def _unique_object(pairs):
    result={}
    for key,value in pairs:
        if key in result:raise Refusal('Duplicate manifest field')
        result[key]=value
    return result


def verify_downloaded_assets(expected, directory, *, require_complete=True):
    """Expected comes from verified build inputs, never the downloaded manifest."""
    directory=Path(directory)
    if directory.is_symlink() or not directory.is_dir():raise Refusal('Downloaded directory must be a real directory')
    tag=expected.get('git_tag','');sha=expected.get('source_sha','')
    if not isinstance(tag,str) or not re.fullmatch(r'v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)',tag):
        raise Refusal('Expected exact stable tag missing')
    if not isinstance(sha,str) or not re.fullmatch(r'[a-f0-9]{40}',sha) or expected.get('version')!=tag[1:]:
        raise Refusal('Expected source/version identity differs')
    declared={}
    for entry in expected.get('distributions',[]):
        name=entry.get('name','');size=entry.get('size');digest=entry.get('sha256','')
        if not isinstance(name,str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9._+-]*',name) or name=='manifest.json' or name in declared:
            raise Refusal('Invalid or duplicate asset name')
        if type(size) is not int or size<0 or size>256*1024*1024 or not isinstance(digest,str) or not re.fullmatch(r'[a-f0-9]{64}',digest):
            raise Refusal('Invalid or unsupported artifact size/hash')
        declared[name]=entry
    if not declared:raise Refusal('No declared distributions')
    present={p.name for p in directory.iterdir()}
    permitted=set(declared)|{'manifest.json'}
    if not present<=permitted:raise Refusal('Undeclared downloaded assets')
    for name in present-set(['manifest.json']):
        entry=declared[name];data=_regular_bytes(directory/name,entry['size'])
        if len(data)!=entry['size'] or hashlib.sha256(data).hexdigest()!=entry['sha256']:
            raise Refusal('Retained artifact bytes differ from verified build')
    if 'manifest.json' in present:
        try:actual=json.loads(_regular_bytes(directory/'manifest.json',65536),object_pairs_hook=_unique_object)
        except (ValueError,UnicodeError) as error:raise Refusal('Malformed downloaded manifest') from error
        if json.dumps(actual,sort_keys=True,separators=(',',':'))!=json.dumps(expected,sort_keys=True,separators=(',',':')):
            raise Refusal('Retained manifest differs from verified build')
    complete=present==permitted
    if require_complete and not complete:raise Refusal('Declared distributions or manifest missing')
    return {'complete':complete,'verified_assets':sorted(present),'git_tag':tag,'source_sha':sha}
