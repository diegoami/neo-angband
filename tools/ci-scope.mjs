#!/usr/bin/env node
/** Fail-full scope selection. Only explicitly reviewed prose/record paths qualify. */
import { execFileSync } from 'node:child_process';
import { readFileSync, appendFileSync } from 'node:fs';
import { pathToFileURL } from 'node:url';

const INERT = new Set([
  'AGENTS.md', 'CLAUDE.md', 'RULES.md', 'PLAN.md',
  'docs/SESSION.md', 'docs/BORG-RS-INTAKE.md', 'docs/ENGINE-SOURCES.md',
  'docs/C-BUGS.md', 'docs/c-fix-handoff.md', 'docs/CI-SCOPING.md',
  'patches/README.md', 'docs/c-bugs/README.md',
]);
// These three documents have executable product contracts; run the real suites.
const CONTRACTS = new Set(['README.md', 'CONTRIBUTING.md', 'docs/INSTALL.md']);
const RECORD = /^(?:docs\/reviews\/[^\0]+\.md|docs\/(?:runs|audit\/c-reports)\/[^\0]+\.(?:md|log)|docs\/(?:runs|audit\/c-reports)\/[^\0]+\/(?:receipt|reuse-receipt|comparison|before-results|after-results|duplicate-search|summary|pins|gates)\.json|docs\/c-bugs\/(?:findings\/CBUG-\d+\.json|catalogue\.json|watch-index\.json)|patches\/(?:upstream-c|upstream-neo)\/[^\0]+\.patch)$/u;
const full = (reason) => ({ full: true, contracts: false, reason });
export function classify(changes) {
  if (!Array.isArray(changes) || changes.length === 0) return full('empty or unavailable diff');
  let contracts = false;
  for (const change of changes) {
    const { path, status, oldMode, newMode } = change;
    if (typeof path !== 'string' || path.startsWith('/') || path.split('/').some(p => p === '..' || p === '.' || p === '') || /[\r\n\0]/u.test(path)) return full('invalid path');
    // Deletions, renames, copies, symlinks and executable files cannot be inert.
    if (!['A', 'M'].includes(status) || newMode !== '100644' || !['000000', '100644'].includes(oldMode)) return full(`non-regular addition/modification: ${path}`);
    if (CONTRACTS.has(path)) { contracts = true; continue; }
    if (!INERT.has(path) && !RECORD.test(path)) return full(`unclassified path: ${path}`);
  }
  return { full: false, contracts, reason: 'only allowlisted regular documentation/evidence records' };
}

const sha = value => typeof value === 'string' && /^[0-9a-f]{40}$/u.test(value) && !/^0+$/u.test(value);
export function eventRange(eventName, event, head) {
  if (!sha(head)) throw new Error('invalid checkout SHA');
  if (eventName === 'pull_request') {
    const base = event.pull_request?.base?.sha;
    const topic = event.pull_request?.head?.sha;
    if (!sha(base) || !sha(topic)) throw new Error('missing PR base/head');
    return { base, head, topic, merge: true };
  }
  if (eventName === 'push' && event.ref === 'refs/heads/master' && sha(event.before) && event.after === head) return { base: event.before, head, merge: false };
  throw new Error('manual, initial push or unsupported event requires full CI');
}

export function parseRaw(raw) {
  const fields = raw.split('\0');
  if (fields.pop() !== '') throw new Error('incomplete raw diff');
  const changes = [];
  for (let i = 0; i < fields.length; i += 2) {
    const m = /^:(\d{6}) (\d{6}) [0-9a-f]{40} [0-9a-f]{40} ([AMDTUXB])$/u.exec(fields[i]);
    if (!m || typeof fields[i + 1] !== 'string') throw new Error('unrecognized raw diff record');
    changes.push({ oldMode: m[1], newMode: m[2], status: m[3], path: fields[i + 1] });
  }
  return changes;
}

export function determine(env = process.env, git = args => execFileSync('git', args, { encoding: 'utf8' })) {
  try {
    const event = JSON.parse(readFileSync(env.GITHUB_EVENT_PATH, 'utf8'));
    const range = eventRange(env.GITHUB_EVENT_NAME, event, env.GITHUB_SHA);
    if (git(['rev-parse', 'HEAD']).trim() !== range.head) throw new Error('checkout does not match event');
    if (range.merge) {
      // Compare the actual tested merge tree to its first parent, not a stale
      // topic-only two-dot diff or the unrelated previous checkout commit.
      const parents = git(['show', '-s', '--format=%P', range.head]).trim().split(' ');
      if (parents.length !== 2 || parents[0] !== range.base || parents[1] !== range.topic) throw new Error('unverified PR merge parents');
    }
    const raw = git(['diff', '--raw', '-z', '--no-renames', '--abbrev=40', range.base, range.head, '--']);
    return { ...classify(parseRaw(raw)), base: range.base, head: range.head };
  } catch (error) { return full(`diff uncertain: ${error.message}`); }
}
if (process.argv[1] && import.meta.url === pathToFileURL(process.argv[1]).href) {
  const result = determine();
  console.log(JSON.stringify(result, null, 2));
  if (process.env.GITHUB_OUTPUT) appendFileSync(process.env.GITHUB_OUTPUT, `full=${result.full}\ncontracts=${result.contracts}\n`);
}
