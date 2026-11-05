#!/usr/bin/env node
// FORK: fail closed when inherited automation loses its repository guard.
import { readFileSync } from "node:fs";
const files = ["release.yml", "publish-npm.yml", "pages.yml", "mod-canary.yml", "edge.yml"];
let count = 0;
for (const file of files) {
  const text = readFileSync(new URL("../.github/workflows/" + file, import.meta.url), "utf8");
  const body = text.split(/^jobs:\s*$/m)[1];
  if (!body) throw new Error(file + ": missing jobs");
  // Deliberately accept only the current simple job structure; new YAML forms
  // require review rather than silently skipping a job during upstream refresh.
  const blocks = body.trimEnd().split(/(?=^  [A-Za-z0-9_-]+:\s*$)/m).filter(s => s.trim());
  for (const block of blocks) {
    const job = /^  ([A-Za-z0-9_-]+):\s*$/m.exec(block)?.[1];
    const conditions = [...block.matchAll(/^    if: (.+)$/gm)];
    const guard = "${{ github.repository == 'neostryder/neo-angband'";
    if (!job || conditions.length !== 1 || !conditions[0][1].startsWith(guard))
      throw new Error(file + ": unguarded or unsupported job " + job);
    const tail = conditions[0][1].slice(guard.length);
    if (!(tail === " }}" || (tail.startsWith(" && (") && tail.endsWith(") }}"))))
      throw new Error(file + ": repository guard must govern the entire condition: " + job);
    count++;
  }
}
console.log(count + " inherited jobs restricted to neostryder/neo-angband; fork tag/manual/release/schedule events skip every job, including always() jobs");
