"""Build staging/identity only. Never calculates versions, creates tags, or publishes."""
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import tarfile
import tomllib

class Refusal(ValueError): pass

def stage(repo, tag, expected_sha, destination):
    repo, destination = Path(repo).resolve(), Path(destination)
    if not re.fullmatch(r'v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)', tag):
        raise Refusal('An exact stable SemVer tag is required')
    if not re.fullmatch(r'[a-f0-9]{40}', expected_sha): raise Refusal('An exact source SHA is required')
    def git(*args):
        try: return subprocess.check_output(['git',*args],cwd=repo,stderr=subprocess.PIPE,timeout=15)
        except (subprocess.SubprocessError,OSError): raise Refusal('Source identity could not be verified') from None
    if git('status','--porcelain').strip(): raise Refusal('Build source must be clean')
    if git('rev-parse','HEAD').decode().strip()!=expected_sha: raise Refusal('Checked-out source differs from requested SHA')
    if git('rev-parse','--verify',f'refs/tags/{tag}^{{commit}}').decode().strip()!=expected_sha:
        raise Refusal('Tag and source SHA differ')
    manifest=git('show',f'{expected_sha}:Cargo.toml').decode()
    parsed=tomllib.loads(manifest)
    package=parsed.get('package',{})
    if not package.get('name') or 'version' in package or 'version' in parsed.get('workspace',{}).get('package',{}):
        raise Refusal('Source must not maintain a project version')
    if git('ls-tree','--name-only',expected_sha,'--','Cargo.lock').strip():
        lock=tomllib.loads(git('show',f'{expected_sha}:Cargo.lock').decode())
        if any(p.get('name')==package['name'] and 'version' in p for p in lock.get('package',[])):
            raise Refusal('Source lockfile must not maintain the project version')
    header=re.compile(r'(?m)^\[package\][ \t]*(?:#.*)?$')
    if len(header.findall(manifest))!=1: raise Refusal('Unsupported package manifest structure')
    version=tag[1:]
    generated=header.sub(lambda m:m.group(0)+'\nversion = '+json.dumps(version),manifest,count=1)
    if destination.exists(): raise Refusal('Build staging destination already exists')
    archive=git('archive','--format=tar',expected_sha)
    destination.mkdir(parents=True)
    with tarfile.open(fileobj=io.BytesIO(archive)) as source: source.extractall(destination,filter='data')
    (destination/'Cargo.toml').write_text(generated)
    inputs={str(p.relative_to(destination)):hashlib.sha256(p.read_bytes()).hexdigest() for p in destination.rglob('*') if p.is_file() and p.name!='Cargo.lock'}
    source_lock=tomllib.loads((destination/'Cargo.lock').read_text()).get('package',[]) if (destination/'Cargo.lock').exists() else []
    identity={'build_inputs':inputs,'dependency_lock':source_lock,'package_name':package['name'],'schema':1,'git_tag':tag,'source_sha':expected_sha,'build_version':version,
              'qualification':{'native_operations':'unsupported','source_authority':False}}
    (destination/'build-identity.json').write_text(json.dumps(identity,indent=2)+'\n')
    return identity

def verify_sdk_dependency(metadata, package_name, repository_url, tag, source_sha):
    """Cargo-reported dependency identity; local overrides are development-only."""
    from urllib.parse import urlsplit,parse_qs
    matches=[p for p in metadata.get('packages',[]) if p.get('name')==package_name]
    if len(matches)!=1: raise Refusal('SDK dependency identity is ambiguous or missing')
    source=matches[0].get('source')
    if not isinstance(source,str) or not source.startswith('git+'):
        raise Refusal('Official SDK dependency must use the pinned Git source')
    parsed=urlsplit(source[4:]);query=parse_qs(parsed.query,keep_blank_values=True)
    actual_url=parsed._replace(query='',fragment='').geturl()
    if actual_url!=repository_url or query!={'tag':[tag]} or parsed.fragment!=source_sha:
        raise Refusal('SDK tag, repository or locked commit differs')
    return {'git_tag':tag,'source_sha':source_sha,'repository_url':repository_url}

def verify_staged_source(destination):
    """Refuse source drift and dependency changes before packaging/publication."""
    destination=Path(destination)
    identity=json.loads((destination/'build-identity.json').read_text())
    expected=identity['build_inputs']
    actual={str(p.relative_to(destination)):hashlib.sha256(p.read_bytes()).hexdigest()
            for p in destination.rglob('*') if p.is_file()
            and str(p.relative_to(destination)).split('/')[0]!='target'
            and str(p.relative_to(destination)) not in ['Cargo.lock','build-identity.json']}
    if actual!=expected: raise Refusal('Staged source changed after exact-tag preparation')
    if (destination/'Cargo.lock').exists():
        lock=tomllib.loads((destination/'Cargo.lock').read_text())
        dependencies=[p for p in lock.get('package',[]) if p.get('name')!=identity['package_name']]
        if dependencies!=identity['dependency_lock']:raise Refusal('Staged dependency lock changed')
    return identity

def distribution_manifest(destination, artifact):
    destination,artifact=Path(destination),Path(artifact)
    identity=verify_staged_source(destination)
    # Cargo rewrites Cargo.toml and preserves the staged manifest as Cargo.toml.orig.
    # Verify packaged bytes, not just the name of an arbitrary file on disk.
    prefix=identity['package_name']+'-'+identity['build_version']+'/'
    try:
        with tarfile.open(artifact,'r:gz') as crate:
            entries={}
            for member in crate.getmembers():
                if not member.name.startswith(prefix): raise Refusal('Crate package root differs')
                if member.isfile():
                    relative=member.name[len(prefix):]
                    if relative in entries: raise Refusal('Duplicate crate member')
                    entries[relative]=crate.extractfile(member).read()
                elif not member.isdir(): raise Refusal('Unsupported crate member')
            package=tomllib.loads(entries['Cargo.toml'].decode())['package']
            if package['name']!=identity['package_name'] or package['version']!=identity['build_version']:
                raise Refusal('Crate package identity differs')
            for name,digest in identity['build_inputs'].items():
                packaged_name='Cargo.toml.orig' if name=='Cargo.toml' else name
                if hashlib.sha256(entries[packaged_name]).hexdigest()!=digest:
                    raise Refusal('Crate source differs from tagged staging')
            # Lockfile identity is checked for the packaged copy as well as staging.
            lock=tomllib.loads(entries['Cargo.lock'].decode())
            dependencies=[p for p in lock.get('package',[]) if p.get('name')!=identity['package_name']]
            if dependencies!=identity['dependency_lock']: raise Refusal('Crate dependency lock differs')
    except (tarfile.TarError,KeyError,UnicodeError,tomllib.TOMLDecodeError) as error:
        raise Refusal('Invalid or incomplete source crate') from error
    return {'schema':1,'git_tag':identity['git_tag'],'source_sha':identity['source_sha'],
            'version':identity['build_version'],
            'distributions':[{'kind':'rust-source-crate','name':artifact.name,
                              'sha256':hashlib.sha256(artifact.read_bytes()).hexdigest(),'size':artifact.stat().st_size}],
            'compatibility':{'rm1':'unqualified','rm2':'unqualified','paper-pro':'unqualified'},
            'native_operations':'unsupported'}

def sdk_build_environment(metadata, package_name, repository_url, tag, source_sha):
    """Build-time runtime identity for an already pinned Git dependency."""
    identity=verify_sdk_dependency(metadata,package_name,repository_url,tag,source_sha)
    if not re.fullmatch(r'v(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)',tag):
        raise Refusal('Build identity requires an exact stable SemVer tag')
    if not re.fullmatch(r'[a-f0-9]{40}',source_sha):raise Refusal('Build identity requires exact source SHA')
    package=next(p for p in metadata['packages'] if p['name']==package_name)
    manifest=Path(package['manifest_path'])
    if manifest.is_symlink():raise Refusal('Consumed package manifest must be a regular source file')
    parsed=tomllib.loads(manifest.read_text())
    source_lock=manifest.parent/'Cargo.lock'
    if source_lock.exists() and any(p.get('name')==package_name and 'version' in p for p in tomllib.loads(source_lock.read_text()).get('package',[])):
        raise Refusal('Consumed source lock must not maintain the project version')
    if parsed.get('package',{}).get('name')!=package_name:
        raise Refusal('Consumed package manifest identity differs')
    if 'version' in parsed.get('package',{}) or 'version' in parsed.get('workspace',{}).get('package',{}):
        raise Refusal('Consumed source must not maintain its own project version')
    try:
        root=subprocess.check_output(['git','rev-parse','--show-toplevel'],cwd=manifest.parent,stderr=subprocess.PIPE,text=True,timeout=15).strip()
        if Path(root).resolve()!=manifest.parent.resolve():raise Refusal('Only a root single-package checkout is qualified')
        actual=subprocess.check_output(['git','rev-parse','HEAD'],cwd=manifest.parent,stderr=subprocess.PIPE,text=True,timeout=15).strip()
        dirty=subprocess.check_output(['git','status','--porcelain','--untracked-files=all','--ignored'],cwd=manifest.parent,stderr=subprocess.PIPE,text=True,timeout=15).strip()
    except (OSError,subprocess.SubprocessError):raise Refusal('Consumed source checkout cannot be verified') from None
    # Cargo 1.98 creates an empty cache-completion marker outside the tagged tree.
    # It is the only generated cache file accepted; ignored developer config is refused.
    marker=manifest.parent/'.cargo-ok'
    if dirty=='?? .cargo-ok' and marker.is_file() and not marker.is_symlink() and marker.stat().st_size==0:
        dirty=''
    if actual!=identity['source_sha'] or dirty:raise Refusal('Consumed cached source differs or changed')
    return {'SDK_BUILD_TAG':tag,'SDK_BUILD_SHA':source_sha,'SDK_BUILD_VERSION':tag[1:]}
