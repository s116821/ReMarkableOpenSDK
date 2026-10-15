"""Actual source eligibility through pinned public PSR; never calculate or mint a version."""
import argparse
from contextlib import chdir
from importlib.metadata import version
from pathlib import Path
import re
import subprocess
import tomllib


def check(repo, source, config):
    if version('python-semantic-release') != '10.7.0':
        raise ValueError('Expected the qualified pinned semantic parser')
    if not re.fullmatch(r'[0-9a-f]{40}', source):
        raise ValueError('Exact source identity required')
    actual = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip()
    if actual != source:
        raise ValueError('Source checkout differs from intended successful commit')
    settings = tomllib.loads(config.read_text())['semantic_release']
    if settings['commit_parser'] != 'conventional-monorepo':
        raise ValueError('Unsupported SDK parser configuration')
    from semantic_release.commit_parser.conventional.parser_monorepo import ConventionalCommitMonorepoParser
    from semantic_release.commit_parser.conventional.options_monorepo import ConventionalCommitMonorepoParserOptions
    from git import Repo
    from semantic_release.enums import LevelBump
    with chdir(repo):
        parser = ConventionalCommitMonorepoParser(ConventionalCommitMonorepoParserOptions(**settings['commit_parser_options']))
    message = Repo(repo).commit(source).message
    parsed = parser.parse_message(message, strict_scope=True)
    if parsed is None or parsed.bump <= LevelBump.NO_RELEASE or not parsed.scope or not parsed.scope.strip():
        raise ValueError('Actual source commit is not an eligible scoped application release')
    return parsed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path.cwd())
    parser.add_argument('--sha', required=True)
    parser.add_argument('--config', type=Path, default=Path(__file__).with_name('python-semantic-release.toml'))
    args = parser.parse_args()
    check(args.repo.resolve(), args.sha, args.config.resolve())
    print('Actual source message admitted by configured upstream semantic parser')


if __name__ == '__main__':
    main()
