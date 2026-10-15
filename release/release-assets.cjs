// Read/download-only adapter around maintained Octokit. Publisher writes remain upstream.
const fs = require('node:fs');
const path = require('node:path');
module.exports = async ({github, context, core}) => {
  const tag = process.env.RELEASE_TAG;
  const source = process.env.RELEASE_SOURCE;
  const directory = process.env.RETAINED_DIRECTORY;
  const final = process.env.REQUIRE_COMPLETE === 'true';
  const {owner, repo} = context.repo;
  const associated = await github.paginate(github.rest.repos.listPullRequestsAssociatedWithCommit,
    {owner, repo, commit_sha:source, per_page:100});
  if (!associated.some(p => p.merged_at && p.merge_commit_sha === source && p.base.ref === 'main'))
    throw new Error('Official publication requires an observed merged main PR at the exact source');
  if (process.env.ELIGIBILITY_ONLY === 'true') return;
  const expected = JSON.parse(fs.readFileSync(process.env.EXPECTED_MANIFEST, 'utf8'));
  if (expected.git_tag !== tag || expected.source_sha !== source) throw new Error('Expected build identity differs');
  fs.mkdirSync(directory, {mode:0o700}); // Never overwrite a retained download directory.
  const id = process.env.RELEASE_ID;
  if (!id) {
    if (final) throw new Error('No release ID exists after upload');
    core.setOutput('published', 'false');
    return;
  }
  if (!/^[1-9][0-9]*$/.test(id) || !Number.isSafeInteger(Number(id))) throw new Error('Invalid exact release ID');
  const {data:release} = await github.rest.repos.getRelease({owner,repo,release_id:Number(id)});
  if (release.id !== Number(id) || release.tag_name !== tag || typeof release.draft !== 'boolean')
    throw new Error('Exact release metadata differs');
  const assets = await github.paginate(github.rest.repos.listReleaseAssets,
    {owner, repo, release_id:release.id, per_page:100});
  const declared = new Map(expected.distributions.map(e => [e.name,e.size]));
  declared.set('manifest.json', fs.statSync(process.env.EXPECTED_MANIFEST).size);
  const seen = new Set();
  for (const asset of assets) {
    if (!declared.has(asset.name) || !/^[A-Za-z0-9][A-Za-z0-9._+-]*$/.test(asset.name) ||
        seen.has(asset.name) || asset.size !== declared.get(asset.name)) throw new Error('Unexpected retained asset identity');
    seen.add(asset.name);
    const downloaded = await github.rest.repos.getReleaseAsset({owner, repo, asset_id:asset.id,
      headers:{accept:'application/octet-stream'}});
    const bytes = Buffer.from(downloaded.data);
    if (bytes.length !== asset.size) throw new Error('Retained asset download size differs');
    fs.writeFileSync(path.join(directory,asset.name),bytes,{flag:'wx',mode:0o600});
  }
  if ((final || !release.draft) && seen.size !== declared.size) throw new Error('Incomplete retained release');
  core.setOutput('published', release.draft ? 'false' : 'true');
};

// Observation validation only; maintained Actions own lookup/create/finalize.
module.exports.observe = (data, tag, expectedId) => {
  if (!data || typeof data.repository !== 'object' || data.repository === null ||
      !Object.hasOwn(data.repository,'release')) throw new Error('Incomplete GraphQL release observation');
  const release = data.repository.release;
  if (release === null) {
    if (expectedId) throw new Error('Expected exact release disappeared');
    return {absent:'true',published:'false',id:''};
  }
  if (!release || typeof release !== 'object' || release.tagName !== tag ||
      !Number.isSafeInteger(release.databaseId) || release.databaseId <= 0 ||
      typeof release.isDraft !== 'boolean' || !release.releaseAssets ||
      !Number.isSafeInteger(release.releaseAssets.totalCount) ||
      release.releaseAssets.totalCount < 0 || release.releaseAssets.totalCount > 5 ||
      (expectedId && String(release.databaseId) !== expectedId)) throw new Error('Invalid exact release observation');
  return {absent:'false',published:String(!release.isDraft),id:String(release.databaseId)};
};
module.exports.created = (data, status, tag) => {
  if (status !== '201' || !data || data.tag_name !== tag || data.draft !== true ||
      !Number.isSafeInteger(data.id) || data.id <= 0) throw new Error('Invalid created draft identity');
  return String(data.id);
};
