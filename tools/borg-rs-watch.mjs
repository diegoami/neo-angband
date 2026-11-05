#!/usr/bin/env node
/** Read-only borg-rs intake: supports file, issues, and migration between them. */
import { execFileSync } from "node:child_process";
import { createHash } from "node:crypto";
import { readFileSync, writeFileSync, mkdirSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
export function readSnapshot(io) {
  const repo = io.repo();
  const head = io.head(repo.default_branch);
  const document = io.document(repo.default_branch);
  const issues = repo.has_issues ? io.issues() : [];
  if (!document && !repo.has_issues) throw new Error("No C bug file or issue feed is available");
  if (issues.length >= 1000) throw new Error("borg-rs issue window reached; intake discovery incomplete");
  return { head: head.sha, mode: document ? (repo.has_issues ? "file-and-issues" : "file") : "issues",
    document: document ? { blob: document.sha, digest: createHash("sha256").update(document.text).digest("hex"), groups: [...document.text.matchAll(/^## \d+\. /gm)].length } : null,
    issues: Object.fromEntries(issues.map((issue) => [issue.number, { title: issue.title, state: issue.state, updatedAt: issue.updatedAt, url: issue.url }])) };
}
export function runWatch({ io, previous = {}, save, report = () => {}, importedDigest }) {
  const next = readSnapshot(io); // Any API/window error occurs before saving.
  report("borg-rs mode: " + next.mode + "; head " + next.head);
  if (next.document) {
    report("C file: " + next.document.groups + " groups; blob " + next.document.blob);
    if (importedDigest && importedDigest !== next.document.digest) report("REVIEW: C file differs from the retained import snapshot; classify/deduplicate new or edited claims.");
  } else report("C file absent; issue feed is available. Preserve file provenance and review the format transition.");
  if (previous.head && previous.head !== next.head) report("MOVED: borg-rs source head (not automatically a new C defect).");
  for (const [number, item] of Object.entries(next.issues)) {
    if (JSON.stringify(previous.issues?.[number]) !== JSON.stringify(item)) report("REVIEW: " + item.url + " — " + item.title + " (classify C vs Rust-only; no automatic import)");
  }
  report("Issue feed: " + Object.keys(next.issues).length + "; direct reports in this C tracker use source:borg-rs and preserve source IDs/pins.");
  report("Mandos owns applicable C→TS translation/adoption; borg-rs owns independent C→Rust translation/adoption. Neither is the Neo mod's fidelity task.");
  save(next);
  return next;
}
if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const root = dirname(dirname(fileURLToPath(import.meta.url)));
  const cache = process.env.ANGBAND_BORG_RS_CACHE ?? join(root,".borg-rs-watch.json");
  const gh = (args) => JSON.parse(execFileSync("gh",args,{encoding:"utf8",stdio:["ignore","pipe","pipe"],timeout:20000}));
  let previous={};try { previous=JSON.parse(readFileSync(cache,"utf8")); } catch {}
  try {
    const catalogue=JSON.parse(readFileSync(join(root,"docs/c-bugs/catalogue.json"),"utf8"));
    runWatch({previous, importedDigest:catalogue.sources.borg_rs.snapshot.sha256, report:console.log,
      save:(data)=>{mkdirSync(dirname(cache),{recursive:true});writeFileSync(cache,JSON.stringify(data,null,2)+"\n");},
      io:{repo:()=>gh(["api","repos/diegoami/borg-rs"]),head:(branch)=>gh(["api","repos/diegoami/borg-rs/commits/"+encodeURIComponent(branch)]),
        document:(branch)=>{try {const d=gh(["api","repos/diegoami/borg-rs/contents/docs/c-bugs.md?ref="+encodeURIComponent(branch)]);return {sha:d.sha,text:Buffer.from(d.content,"base64").toString("utf8")};}catch(error){if(String(error.stderr??error).includes("404"))return null;throw error;}},
        issues:()=>gh(["issue","list","-R","diegoami/borg-rs","--state","all","--limit","1000","--json","number,title,state,updatedAt,url"])} });
  } catch(error) {console.error("INCOMPLETE: "+error.message+"; previous successful baseline retained.");process.exitCode=1;}
}
