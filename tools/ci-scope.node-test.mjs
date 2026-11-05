import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtempSync, writeFileSync, rmSync, readFileSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { classify, eventRange, parseRaw, determine } from './ci-scope.mjs';
const change = (path, extra = {}) => ({ path, status: 'M', oldMode: '100644', newMode: '100644', ...extra });
const base = 'a'.repeat(40), head = 'b'.repeat(40), topic = 'c'.repeat(40);

test('only reviewed inert regular records take the small lane', () => {
  for (const path of ['AGENTS.md', 'RULES.md', 'docs/SESSION.md', 'docs/reviews/verification.md', 'docs/audit/c-reports/CBUG-0002/after-knockout.log', 'docs/audit/c-reports/CBUG-0002/receipt.json', 'docs/c-bugs/findings/CBUG-0002.json', 'patches/upstream-c/proposal.patch']) assert.equal(classify([change(path)]).full, false, path);
});
test('README, CONTRIBUTING and installation prose select real doc contracts', () => {
  for (const path of ['README.md', 'CONTRIBUTING.md', 'docs/INSTALL.md']) assert.deepEqual(classify([change(path)]), { full: false, contracts: true, reason: 'only allowlisted regular documentation/evidence records' });
});
test('runtime, tests, dependencies, workflows, config, unknown docs and runnable evidence stay full', () => {
  for (const path of ['packages/core/src/game/trap.ts', 'packages/core/src/game/trap.test.ts', 'tests/e2e/play.spec.ts', 'package.json', 'pnpm-lock.yaml', 'pnpm-workspace.yaml', 'tsconfig.json', '.github/workflows/ci.yml', '.github/actions/private-scan/action.yml', 'tools/ci-scope.mjs', 'tools/vitest-doc-contracts.config.mjs', 'docs/modding/API2.md', 'docs/new.md', 'docs/audit/c-reports/CBUG-0002/scenario.c', 'docs/audit/c-reports/CBUG-0002/run-modes.py', 'docs/audit/c-reports/CBUG-0002/borg.txt', 'docs/audit/c-reports/CBUG-0002/config.json', 'reference/src/borg.c']) assert.equal(classify([change(path)]).full, true, path);
  assert.equal(classify([change('AGENTS.md'), change('package.json')]).full, true);
});
test('empty, removed, renamed, executable, symlink and malformed paths stay full', () => {
  assert.equal(classify([]).full, true);
  for (const extra of [{ status: 'D', newMode: '000000' }, { status: 'R' }, { newMode: '100755' }, { oldMode: '120000' }, { newMode: '120000' }]) assert.equal(classify([change('AGENTS.md', extra)]).full, true);
  for (const path of ['../AGENTS.md', '/AGENTS.md', 'docs/../AGENTS.md', 'docs//SESSION.md', 'AGENTS.md\nother']) assert.equal(classify([change(path)]).full, true);
});
test('event ranges use exact PR merge base/head and push before/after; manual/initial/unsupported fail full', () => {
  assert.deepEqual(eventRange('pull_request', { pull_request: { base: { sha: base }, head: { sha: topic } } }, head), { base, head, topic, merge: true });
  assert.deepEqual(eventRange('push', { ref: 'refs/heads/master', before: base, after: head }, head), { base, head, merge: false });
  for (const [name, event] of [['workflow_dispatch', {}], ['issues', {}], ['push', { ref: 'refs/heads/master', before: '0'.repeat(40), after: head }], ['push', { ref: 'refs/heads/task/foo', before: base, after: head }], ['push', { ref: 'refs/heads/master', before: base, after: topic }], ['pull_request', {}]]) assert.throws(() => eventRange(name, event, head));
});
test('raw parser retains filenames and rejects truncation or unsupported records', () => {
  const raw = `:100644 100644 ${base} ${head} M\0docs/reviews/space in name.md\0`;
  assert.equal(parseRaw(raw)[0].path, 'docs/reviews/space in name.md');
  assert.throws(() => parseRaw(raw.slice(0, -1)));
  assert.throws(() => parseRaw(`:100644 100644 ${base} ${head} R100\0a\0b\0`));
});
test('real git PR merge and push changes classify correctly, with uncertainty controls', () => {
  const root = mkdtempSync(join(tmpdir(), 'neo-ci-scope-'));
  const git = args => execFileSync('git', args, { cwd: root, encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] });
  try {
    git(['init', '-b', 'master']); git(['config', 'user.name', 'CI fixture']); git(['config', 'user.email', 'fixture@example.invalid']);
    writeFileSync(join(root, 'AGENTS.md'), 'base'); git(['add', '.']); git(['commit', '-m', 'base']);
    const before = git(['rev-parse', 'HEAD']).trim();
    git(['checkout', '-b', 'topic']); writeFileSync(join(root, 'AGENTS.md'), 'docs'); git(['commit', '-am', 'docs']);
    const topicHead = git(['rev-parse', 'HEAD']).trim();
    git(['checkout', 'master']); git(['merge', '--no-ff', 'topic', '-m', 'merge']);
    const mergeHead = git(['rev-parse', 'HEAD']).trim(), eventPath = join(root, 'event.json');
    const env = { GITHUB_EVENT_PATH: eventPath, GITHUB_EVENT_NAME: 'pull_request', GITHUB_SHA: mergeHead };
    writeFileSync(eventPath, JSON.stringify({ pull_request: { base: { sha: before }, head: { sha: topicHead } } }));
    assert.equal(determine(env, git).full, false);
    writeFileSync(eventPath, JSON.stringify({ pull_request: { base: { sha: before }, head: { sha: before } } }));
    assert.equal(determine(env, git).full, true, 'mismatched merge parents');
    writeFileSync(eventPath, JSON.stringify({ ref: 'refs/heads/master', before, after: mergeHead }));
    assert.equal(determine({ ...env, GITHUB_EVENT_NAME: 'push' }, git).full, false);
    writeFileSync(eventPath, JSON.stringify({ ref: 'refs/heads/master', before: 'f'.repeat(40), after: mergeHead }));
    assert.equal(determine({ ...env, GITHUB_EVENT_NAME: 'push' }, git).full, true, 'unavailable base');
    assert.equal(determine(env, () => { throw new Error('git failed'); }).full, true);
    writeFileSync(eventPath, '{broken'); assert.equal(determine(env, git).full, true);
  } finally { rmSync(root, { recursive: true, force: true }); }
});
test('workflows keep full gates, no branch-push duplicate or issue events, and scoped metadata coverage', () => {
  const read = name => readFileSync(new URL(`../.github/workflows/${name}`, import.meta.url), 'utf8');
  const ci = read('ci.yml'), metadata = read('fork-session-docs.yml');
  assert.match(ci, /branches: \[master\]/u);
  assert.match(metadata, /branches: \[master, workbench\/c-borg\]/u);
  for (const text of [ci, metadata]) {
    assert.match(text, /pull_request:/u); assert.match(text, /workflow_dispatch:/u);
    assert.match(text, /cancel-in-progress: \$\{\{ github.event_name == 'pull_request' \}\}/u);
    assert.doesNotMatch(text, /(?:issues|issue_comment|pull_request_target):/u);
  }
  assert.equal((ci.match(/needs.scope.outputs.full != 'false'/gu) ?? []).length, 2);
  for (const command of ['pnpm build', 'pnpm lint', 'pnpm test', 'pnpm check:npm', 'xvfb-run -a node tools/play-smoke.mjs --no-sandbox']) assert.ok(ci.includes(command), command);
  assert.ok(metadata.includes('node tools/private-scan.mjs'));
  const contracts = readFileSync(new URL('./vitest-doc-contracts.config.mjs', import.meta.url), 'utf8');
  for (const file of ['packages/desktop/src/packaging.test.ts', 'packages/core/src/toolchain.test.ts']) assert.ok(contracts.includes(file));
});
