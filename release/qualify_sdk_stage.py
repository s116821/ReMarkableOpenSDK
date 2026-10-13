"""Build reviewed SDK bytes in an isolated fixture; never mint an official tag."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import tarfile
import tempfile
from stage_source import stage, verify_staged_source, distribution_manifest


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',required=True,type=Path)
    parser.add_argument('--expected-source',required=True)
    parser.add_argument('--cargo',default='cargo')
    args=parser.parse_args();source=args.source.resolve()
    def source_git(*cmd):
        return subprocess.check_output(['git',*cmd],cwd=source,stderr=subprocess.PIPE).decode().strip()
    real_sha=source_git('rev-parse','HEAD')
    if real_sha!=args.expected_source or source_git('status','--porcelain'):
        raise ValueError('Qualification source is not exact and clean')
    real_tree=source_git('rev-parse','HEAD^{tree}')
    before=(source/'Cargo.lock').read_bytes()
    archive=subprocess.check_output(['git','archive','--format=tar',real_sha],cwd=source,stderr=subprocess.PIPE)
    with tempfile.TemporaryDirectory(prefix='sdk-real-tree-owned-') as directory:
        root=Path(directory);fixture=root/'source';fixture.mkdir();output=root/'stage'
        (root/'.owned-fixture').write_text('Source-tree qualification only; no official tag/release')
        with tarfile.open(fileobj=io.BytesIO(archive)) as tree:tree.extractall(fixture,filter='data')
        config=root/'empty.gitconfig';config.write_text('')
        env={**os.environ,'GIT_CONFIG_NOSYSTEM':'1','GIT_CONFIG_GLOBAL':str(config),
             'GIT_AUTHOR_NAME':'Owned Fixture','GIT_COMMITTER_NAME':'Owned Fixture',
             'GIT_AUTHOR_EMAIL':'fixture@example.invalid','GIT_COMMITTER_EMAIL':'fixture@example.invalid'}
        def git(*cmd):
            return subprocess.check_output(['git',*cmd],cwd=fixture,env=env,stderr=subprocess.PIPE,text=True).strip()
        git('init','--initial-branch=main');git('config','core.autocrlf','false');git('add','.');git('commit','-m','test: exact SDK tree fixture')
        if git('rev-parse','HEAD^{tree}')!=real_tree:raise ValueError('Fixture tree differs from reviewed source')
        fixture_sha=git('rev-parse','HEAD');fixture_tag='v0.3.7';git('tag',fixture_tag)
        identity=stage(fixture,fixture_tag,fixture_sha,output)
        subprocess.run([args.cargo,'generate-lockfile','--offline'],cwd=output,env=env,check=True)
        verify_staged_source(output)
        subprocess.run([args.cargo,'package','--locked','--offline'],cwd=output,env=env,check=True)
        artifact=output/'target'/'package'/f"{identity['package_name']}-{identity['build_version']}.crate"
        manifest=distribution_manifest(output,artifact)
        if (source/'Cargo.lock').read_bytes()!=before or source_git('status','--porcelain'):
            raise ValueError('Qualification changed source')
        print(json.dumps({'qualification':'reviewed-source-tree-fixture','reviewed_source_sha':real_sha,
                          'reviewed_tree':real_tree,'fixture_source_sha':fixture_sha,'fixture_tag':fixture_tag,
                          'external_packages':len(identity['dependency_lock']),
                          'artifact_sha256':manifest['distributions'][0]['sha256'],
                          'official_release':False,'native_operations':'unsupported'},sort_keys=True))


if __name__=='__main__':main()
