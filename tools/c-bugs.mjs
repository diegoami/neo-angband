#!/usr/bin/env node
/** Local C-bug catalogue renderer. Offline; never posts, fetches or applies patches. */
import { validateSourceSnapshot } from "./c-bugs-source.mjs";
import { readFileSync, writeFileSync, existsSync, readdirSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { join, dirname } from "node:path";

const root = dirname(dirname(fileURLToPath(import.meta.url)));
const input = join(root, "docs/c-bugs/catalogue.json");
const output = join(root, "docs/C-BUGS.md");
const data = JSON.parse(readFileSync(input, "utf8"));
const files = readdirSync(join(root, "docs/c-bugs/findings")).filter((name) => name.endsWith(".json")).sort();
if (JSON.stringify(files) !== JSON.stringify([...data.finding_files].sort())) throw new Error("Finding inventory differs from per-finding files");
if (files.some((name) => !/^CBUG-\d{4}\.json$/.test(name))) throw new Error("Invalid finding filename");
data.findings = data.finding_files.map((name) => JSON.parse(readFileSync(join(root, "docs/c-bugs/findings", name), "utf8")));
const fail = (message) => { throw new Error("c-bugs: " + message); };
if (data.schema !== "angband/c-bugs@3") fail("unsupported schema");
if (data.policy.upstream_c_submissions !== "paused" || data.policy.automatic_upstream_posts !== false) fail("policy must preserve the owner's C submission pause");
const referenceRoles = new Set(["c", "neo", "mandos", "borg_rs"]);
function checkReference(ref, id, baseline = true) {
  if (!ref || !referenceRoles.has(ref.role) || !/^[0-9a-f]{40}$/.test(ref.commit)
    || !Number.isInteger(ref.start) || !Number.isInteger(ref.end)
    || ref.start < 1 || ref.end < ref.start || !ref.path || ref.path.includes(".."))
    fail("missing/invalid pinned line reference " + id);
  const source = ref.role === "neo" ? data.sources.neo_borg : data.sources[ref.role];
  if (baseline && (ref.repository !== source.repository || ref.commit !== source.commit))
    fail("reference disagrees with source pin " + id);
}
const groups = new Map();
for (const group of data.groups) {
  if (groups.has(group.number)) fail("duplicate source group " + group.number);
  checkReference(group.discovery_reference, "group " + group.number);
  groups.set(group.number, group);
}
if (groups.size !== data.sources.borg_rs.groups) fail("source group count differs");
for (let n = 1; n <= groups.size; n++) if (!groups.has(n)) fail("missing source group " + n);
const ids = new Set();
const ownerIssues = new Set();
const levels = new Set(["reported-lead", "source-checked", "c-game-reproduced", "c-game-flags-only"]);
const statuses = new Set(["investigate", "local-proposal", "held-impact", "review-only"]);
for (const finding of data.findings) {
  if (finding.origin !== "upstream-c") fail("port omissions belong on internal issues, not this C catalogue: " + finding.id);
  if (!/^CBUG-\d{4}$/.test(finding.id) || ids.has(finding.id)) fail("invalid/duplicate finding " + finding.id);
  ids.add(finding.id);
  if (finding.tracker?.repository !== "diegoami/neo-angband-tracker" || !Number.isInteger(finding.tracker.number) || finding.tracker.number < 1) fail("unqualified/wrong source issue owner " + finding.id);
  if (ownerIssues.has(finding.tracker.number)) fail("two finding records share a source issue " + finding.id);
  ownerIssues.add(finding.tracker.number);
  if (!finding.references?.some((ref) => ref.role === "c")) fail("no C code reference " + finding.id);
  for (const ref of finding.references) checkReference(ref, finding.id);
  if (!levels.has(finding.evidence_at_import) || !statuses.has(finding.status_at_import)) fail("unknown evidence/status " + finding.id);
  if (!["high", "med", "low"].includes(finding.priority)) fail("unknown priority " + finding.id);
  for (const number of finding.source_groups) if (!groups.has(number)) fail("unknown source group " + finding.id);
  if (finding.evidence_path && !existsSync(join(root, finding.evidence_path))) fail("missing evidence " + finding.id);
  if (finding.patch && !existsSync(join(root, finding.patch))) fail("missing patch " + finding.id);
  for (const pr of finding.proposals_at_import) {
    if (!pr.repository.startsWith(data.owner + "/")) fail("C fix proposal is outside our repositories: " + finding.id);
    if (pr.url !== "https://github.com/" + pr.repository + "/pull/" + pr.number) fail("PR URL/repository mismatch " + finding.id);
    if (pr.repository === data.policy.local_pr_base.repository && pr.base !== data.policy.local_pr_base.branch) fail("C-source PR has wrong base " + finding.id);
    if (pr.repository === data.policy.local_pr_base.repository && (!pr.branch || !/^[A-Za-z0-9][A-Za-z0-9._/-]*$/.test(pr.branch))) fail("C-source proposal needs a branch for its compare link: " + finding.id);
  }
}
for (const group of data.groups) for (const id of group.findings) {
  const finding = data.findings.find((item) => item.id === id);
  if (!finding || !finding.source_groups.includes(group.number)) fail("broken group/finding link " + id);
}
for (const finding of data.findings) for (const n of finding.source_groups) {
  if (!groups.get(n).findings.includes(finding.id)) fail("missing reverse group link " + finding.id);
}
if (data.related_port_leads?.length) fail("port leads must stay on internal issues, not the C community list");
for (const claim of data.report_coverage.source_claims) {
  if (!groups.has(claim.group) || claim.start < 1 || claim.end < claim.start || !claim.finding_ids.length)
    fail("invalid reported-claim coverage");
  for (const id of claim.finding_ids) if (!ids.has(id)) fail("reported claim has no finding: " + id);
}
for (const group of data.groups) if (!data.report_coverage.source_claims.some((claim) => claim.group === group.number))
  fail("uncovered source group " + group.number);

validateSourceSnapshot(data, readFileSync(join(root, data.sources.borg_rs.snapshot.path), "utf8"));

const issueBoard = "https://github.com/diegoami/neo-angband-tracker/issues?q=is%3Aissue+label%3Aupstream-c+label%3Afinding";
const rendered = [
  "# Angband C Borg findings and candidates", "",
  "## Easy method: open a PR from an existing C proposal branch", "",
  "[Compare the map-dump correction with Angband master](https://github.com/angband/angband/compare/master...diegoami:neo-angband-tracker:pr/c-borg-map-dump-cancel)", "",
  "Review the diff and choose **Create pull request** if your account has the required permissions. Opening the comparison submits nothing. [Copy/patch alternatives and follow-up details](c-fix-handoff.md).", "",
  "## The issue list is authoritative", "",
  "[Browse all C findings and candidates](" + issueBoard + "). Every finding has its own issue with source lines, discovery credit, evidence confidence and proposal links. This page does not duplicate that list or its live state.", "",
  "- [Open C findings](" + issueBoard + "+is%3Aopen)",
  "- [Reported or unproven candidates](" + issueBoard + "+label%3Acandidate)",
  "- [Native C-game reproductions](" + issueBoard + "+label%3Aevidence%3Ac-game-reproduced)", "",
  "Reported candidates are not silently promoted to verified defects. Source issue open/closed state concerns the source correction; downstream carried/fixed state belongs only in linked Mandos adoption issues.", "",
  "## Scope and supporting records", "",
  "This tracker covers original Angband, especially its C Borg, alongside Neo Angband engine repairs. Neo Borg port defects belong in [our mod tracker](https://github.com/diegoami/neo-angband-borg-mod-tracker/issues).", "",
  "[Per-finding import/provenance records](c-bugs/findings), [source snapshot](audit/c-reports/borg-rs-c-bugs.md), native scenario reports and [proposal patches](../patches) preserve evidence. They are historical import/audit material, not a second editable issue-state ledger.", "",
  "Use the issue to update investigation, evidence conclusions, current proposals and resolution. The split JSON records preserve the imported source correspondence; regenerate this landing page and the data-only watch index with `node tools/c-bugs.mjs --write`, verify with `--check`.", "",
  "No angband/angband issues, PRs or comments are submitted by us under the standing owner pause. See [AGENTS](../AGENTS.md), [RULES](../RULES.md) and the [session map](SESSION.md).", ""
].join("\n");
const watch = JSON.stringify({ schema: "angband/c-bugs-watch@1", sources: data.sources,
  findings: data.findings.map((finding) => ({ id: finding.id, tracker: finding.tracker,
    references: finding.references.filter((ref) => ref.role === "c").map((ref) => ({ role: "c", path: ref.path })) })) }, null, 2) + "\n";
const watchPath = join(root, "docs/c-bugs/watch-index.json");
if (process.argv.includes("--write")) { writeFileSync(output, rendered); writeFileSync(watchPath, watch); }
else if (!process.argv.includes("--check")) fail("use --write or --check");
else if (!existsSync(output) || readFileSync(output,"utf8") !== rendered || !existsSync(watchPath) || readFileSync(watchPath,"utf8") !== watch) fail("generated landing page/index is stale; run --write");
console.log("c-bugs: " + data.findings.length + " individually tracked import records; source coverage valid; issue list owns live state");
