#!/usr/bin/env node
/** Read-only upstream/carry audit. See docs/UPSTREAM.md and process sections 6–7. */
import { execFileSync } from "node:child_process";
import { readFileSync, writeFileSync, unlinkSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const TRACKERS = (process.env.ANGBAND_TRACKER_REPOS ?? "diegoami/neo-angband-tracker,diegoami/neo-angband-borg-mod-tracker,diegoami/mandos").split(",").filter(Boolean);
const CARRY_LABELS = ["fixed", "carried", "not-carried", "partially-carried"];
const root = dirname(dirname(fileURLToPath(import.meta.url)));
const cachePath = process.env.MANDOS_UPSTREAM_WATCH_CACHE ?? join(root, ".upstream-watch.json");
const apiCache = new Map();
const ghJson = (args) => {
  const key = JSON.stringify(args);
  if (args[0] === "api" && apiCache.has(key)) return apiCache.get(key);
  const result = JSON.parse(execFileSync("gh", args, { encoding: "utf8", stdio: ["ignore", "pipe", "ignore"] }));
  if (args[0] === "api") apiCache.set(key, result);
  return result;
};
const canonicalRepo = (repo) => repo === "diegoami/neo-angband" ? "diegoami/neo-angband-tracker"
  : repo === "diegoami/neo-angband-mod-borg" ? "diegoami/neo-angband-borg-mod-tracker" : repo;
const labelsOf = (issue) => issue.labels.map((label) => typeof label === "string" ? label : label.name);

if (process.argv.includes("--reset")) {
  try { unlinkSync(cachePath); } catch { /* no baseline */ }
}
let previous = {};
try { previous = JSON.parse(readFileSync(cachePath, "utf8")); } catch { /* first run */ }

const tracking = [];
const problems = [];
let discoveryComplete = true;
for (const repository of TRACKERS) {
  try {
    const issues = ghJson(["issue", "list", "--repo", repository, "--state", "all", "--limit", "1000", "--json", "number,title,body,labels,state,updatedAt"]);
    if (issues.length >= 1000) { problems.push(repository + ": issue window reached"); discoveryComplete = false; }
    tracking.push(...issues.filter((issue) => labelsOf(issue).some((label) => label === "upstream" || label === "upstream-c" || (repository === "diegoami/mandos" && label === "integration")))
      .filter((issue) => issue.state !== "CLOSED" || labelsOf(issue).includes("patch-active"))
      .map((issue) => ({ ...issue, repository })));
  } catch {
    discoveryComplete = false;
    problems.push(repository + ": tracker discovery failed; previous baseline retained");
  }
}

const threads = [];
const candidates = [];
for (const issue of tracking) {
  const consumer = issue.repository === "diegoami/mandos";
  const selected = CARRY_LABELS.filter((label) => labelsOf(issue).includes(label));
  const carry = consumer ? (selected.length === 1 ? selected[0] : selected.length === 0 ? "MISSING" : "CONFLICT") : "source";
  if (consumer && selected.length !== 1) problems.push(`${issue.repository}#${issue.number}: carry status ${carry}${selected.length ? ` (${selected.join(", ")})` : ""}`);
  if (!consumer && (selected.length || labelsOf(issue).includes("patch-active"))) problems.push(`${issue.repository}#${issue.number}: source issue has consumer-state labels`);
  if (consumer && carry === "fixed" && issue.state === "OPEN") problems.push(`${issue.repository}#${issue.number}: fixed consumer tracker is still open; review closure`);
  const local = { number: issue.number, carry, repository: issue.repository, what: `${issue.repository}#${issue.number}: ${issue.title}` };
  const line = issue.body.split("\n").find((text) => text.startsWith("**Threads**:"));
  if (consumer && line === undefined) problems.push(`${issue.repository}#${issue.number}: missing **Threads**: line (use 'none yet' for an internal candidate)`);
  const refs = [...(line ?? "").matchAll(/([\w.-]+\/[\w.-]+)#(\d+)/g)];
  if (consumer && refs.length === 0) candidates.push(local);
  for (const match of refs) threads.push({ ...local, repo: canonicalRepo(match[1]), num: Number(match[2]) });
}

const rows = [];
// Source-board activity is observed directly from the discovery response;
// new file/issue/direct reports need not already have external thread links.
for (const issue of tracking.filter((item) => item.repository !== "diegoami/mandos")) {
  const key = issue.repository + "#" + issue.number;
  const before = previous[key];
  const updatedAt = issue.updatedAt ?? null;
  const state = String(issue.state).toUpperCase();
  rows.push({ repo: issue.repository, num: issue.number, kind: "source", carry: "source",
    what: issue.repository + "#" + issue.number + ": " + issue.title, state,
    comments: null, head: null, updatedAt,
    moved: before !== undefined && (before.state !== state || before.updatedAt !== updatedAt) });
}

for (const thread of threads) {
  const key = `${thread.repo}#${thread.num}`;
  try {
    const issue = ghJson(["api", `repos/${thread.repo}/issues/${thread.num}`]);
    const pr = issue.pull_request ? ghJson(["api", `repos/${thread.repo}/pulls/${thread.num}`]) : null;
    const state = pr?.merged_at ? "MERGED" : String(issue.state).toUpperCase();
    const head = pr?.head?.sha ?? null;
    const before = previous[key];
    const moved = before !== undefined && (before.state !== state || before.comments !== issue.comments
      || (before.updatedAt !== undefined && before.updatedAt !== issue.updated_at)
      || (before.head !== undefined && before.head !== head));
    rows.push({ ...thread, kind: pr ? "pr" : "issue", state, head, comments: issue.comments, updatedAt: issue.updated_at, moved });
  } catch {
    problems.push(`${key}: upstream API read failed; previous baseline retained`);
    rows.push({ ...thread, kind: "?", state: "UNKNOWN", moved: false, error: true });
  }
}

if (tracking.length > 0 && discoveryComplete) {
  const cache = {};
  for (const row of rows) {
    const key = `${row.repo}#${row.num}`;
    if (row.error) { if (previous[key]) cache[key] = previous[key]; }
    else cache[key] = { state: row.state, comments: row.comments, updatedAt: row.updatedAt, head: row.head };
  }
  writeFileSync(cachePath, JSON.stringify(cache, null, 2) + "\n");
}

const pad = (value, width) => String(value).padEnd(width);
console.log(pad("thread", 42) + pad("upstream", 10) + pad("Consumer state", 20) + pad("moved", 7) + "tracker");
for (const row of rows) {
  console.log(pad(`${row.repo}#${row.num} ${row.kind}`, 42) + pad(row.state, 10)
    + pad(row.carry, 20) + pad(row.moved ? "YES" : "", 7) + row.what);
}
if (candidates.length) {
  console.log("\nTrackers without external issue/PR links (may include accepted-commit patches):");
  for (const candidate of candidates) console.log(`${pad(`${candidate.repository}#${candidate.number}`, 48)}${pad(candidate.carry, 20)}${candidate.what}`);
}
console.log(`\nAudited ${tracking.length} trackers (open or patch-active); ${threads.length} upstream references; ${candidates.length} internal candidates.`);
const moved = rows.filter((row) => row.moved);
console.log(moved.length
  ? `MOVED: ${moved.map((row) => `${row.repo}#${row.num}`).join(", ")} — inspect the changes and update our tracker in this pass; activity may be ours, a maintainer's or a bot's.`
  : rows.some((row) => row.error) || process.exitCode || problems.length
    ? "No confirmed movement; the upstream audit is incomplete."
    : "No upstream movement since the last baseline.");
console.log("Source issue state and linked Mandos consumer state are distinct. Carried stays open until upstream correction is adopted downstream and verified; this report never changes labels or removes patches.");
if (problems.length) {
  console.log("\nAudit problems:\n" + problems.map((problem) => `- ${problem}`).join("\n"));
  process.exitCode = 1;
}
