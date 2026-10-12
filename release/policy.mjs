// PR titles become squash commit subjects; docs never cause a release.
const patchTypes = ['build', 'ci', 'chore', 'refactor', 'test', 'revert'];
export const analyzer = {
  preset: 'conventionalcommits',
  releaseRules: [
    {type: 'docs', release: false},
    ...patchTypes.flatMap(type => [{type, release: 'patch'}, {type, breaking: true, release: 'major'}]),
  ],
};
