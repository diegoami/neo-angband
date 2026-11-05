/** Offline completeness checks against the exact fetched discovery document. */
import { createHash } from "node:crypto";
export function validateSourceSnapshot(data, source) {
  const fail = (message) => { throw new Error("c-bugs source: " + message); };
  const pin = data.sources.borg_rs;
  if (pin.snapshot.commit !== pin.commit) fail("snapshot pin differs from discovery pin");
  if (createHash("sha256").update(source).digest("hex") !== pin.snapshot.sha256) fail("snapshot digest differs");
  const lines = source.split("\n");
  const headings = lines.flatMap((line) => { const match = /^## (\d+)\. /.exec(line); return match ? [Number(match[1])] : []; });
  if (JSON.stringify(headings) !== JSON.stringify(data.groups.map((group) => group.number))) fail("source groups are missing, extra or reordered");
  const coverage = data.report_coverage;
  if (coverage.leaf_reports !== coverage.source_claims.length) fail("source report count differs");
  const excluded = new Map();
  for (const entry of coverage.non_report_bullets) {
    if (excluded.has(entry.line) || !entry.reason || lines[entry.line - 1] !== entry.text) fail("invalid non-report bullet classification");
    excluded.set(entry.line, entry);
  }
  for (const finding of data.findings) for (const report of finding.reported_sources ?? []) {
    const ref = report.reference;
    if (ref.commit !== pin.commit || !lines.slice(ref.start - 1, ref.end).join("\n").includes(report.text.trim())) fail("reported text/range differs: " + finding.id);
  }
  for (let index = 0; index < lines.length; index++) {
    if (!/^\s*\* /.test(lines[index])) continue;
    const number = index + 1;
    const covered = coverage.source_claims.some((claim) => claim.start <= number && number <= claim.end);
    if (!covered && !excluded.has(number)) fail("unclassified source bullet at line " + number);
    if (covered && excluded.has(number)) fail("source bullet is both report and non-report");
  }
}
