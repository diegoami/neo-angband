import assert from "node:assert/strict";
import { test } from "node:test";
import { createHash } from "node:crypto";
import { validateSourceSnapshot } from "./c-bugs-source.mjs";
const source = "## 1. Example\n* A reported defect.\n* Rust: retained.\n";
function data(text=source) { return { sources:{borg_rs:{commit:"pin",snapshot:{commit:"pin",sha256:createHash("sha256").update(text).digest("hex")}}}, groups:[{number:1}], findings:[{id:"example",reported_sources:[{reference:{commit:"pin",start:2,end:2},text:"A reported defect."}]}], report_coverage:{leaf_reports:1,source_claims:[{start:2,end:2}],non_report_bullets:[{line:3,text:"* Rust: retained.",reason:"Rust note"}]}}; }
test("accepts explicitly classified source",()=>assert.doesNotThrow(()=>validateSourceSnapshot(data(),source)));
test("rejects an appended group absent from inventory",()=>{ const s=source+"## 2. New\n* report\n";assert.throws(()=>validateSourceSnapshot(data(s),s),/source groups/); });
test("rejects unclassified bullets within an existing group",()=>{ const s=source+"* New report\n";assert.throws(()=>validateSourceSnapshot(data(s),s),/unclassified source bullet/); });
test("rejects stale pins, bytes and excerpt ranges",()=>{const d=data();d.sources.borg_rs.commit="other";assert.throws(()=>validateSourceSnapshot(d,source),/snapshot pin/);assert.throws(()=>validateSourceSnapshot(data(),source+"changed"),/digest/);const e=data();e.findings[0].reported_sources[0].reference.start=3;assert.throws(()=>validateSourceSnapshot(e,source),/reported text/);});
