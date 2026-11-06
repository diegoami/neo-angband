#!/usr/bin/env node
/**
 * Regenerate the C Borg defect index and hotspot tables in
 * docs/c-borg/BORG_SPEC.md from the canonical catalogue.
 *
 * The index is built from the `site` field of every record in
 * docs/c-bugs/findings/CBUG-NNNN.json.  The board and the catalogue own
 * the live state; this script only produces the reverse lookup, so the
 * table in the specification can never disagree with them by accident.
 *
 * Usage:
 *   node tools/borg-defect-index.mjs --check   # verify the spec matches
 *   node tools/borg-defect-index.mjs --write   # rewrite the spec sections
 *
 * The sections it owns are marked in the specification with
 *   <!-- borg-defect-index:start --> ... <!-- borg-defect-index:end -->
 * and
 *   <!-- borg-defect-hotspots:start --> ... <!-- borg-defect-hotspots:end -->
 */

import { readFileSync, writeFileSync } from "node:fs";
import { globSync } from "node:fs";
import { join, dirname } from "node:path";
import { fileURLToPath } from "node:url";

const ROOT = join(dirname(fileURLToPath(import.meta.url)), "..");
const SPEC = join(ROOT, "docs/c-borg/BORG_SPEC.md");
const FINDINGS = join(ROOT, "docs/c-bugs/findings");

const args = process.argv.slice(2);
const mode = args.includes("--write") ? "write" : "check";

/* ------------------------------------------------------------------ */

function loadFindings() {
	const records = [];
	for (const path of globSync(join(FINDINGS, "CBUG-*.json")).sort()) {
		records.push(JSON.parse(readFileSync(path, "utf8")));
	}
	return records;
}

function siteEntries(record) {
	const site = record.site ?? "";
	const parts = site.split(" / ").map((p) => p.trim()).filter(Boolean);
	const out = [];
	for (const part of parts.length ? parts : ["(unsited)"]) {
		const colon = part.indexOf(":");
		const file = (colon === -1 ? part : part.slice(0, colon)).trim();
		const fn = (colon === -1 ? "(file level)" : part.slice(colon + 1).trim()) || "(file level)";
		out.push({ file, fn });
	}
	return out;
}

function issueNumber(record) {
	return record.tracker?.number ?? "?";
}

/* ------------------------------------------------------------------ */

function buildIndex(records) {
	const byFile = new Map();
	for (const record of records) {
		for (const { file, fn } of siteEntries(record)) {
			if (!byFile.has(file)) byFile.set(file, new Map());
			const fns = byFile.get(file);
			if (!fns.has(fn)) fns.set(fn, []);
			fns.get(fn).push(record);
		}
	}

	const files = [...byFile.keys()].sort((a, b) => a.toLowerCase().localeCompare(b.toLowerCase()));
	let entries = 0;

	const lines = [];
	lines.push("### 11.13 Complete defect index by source site");
	lines.push("");
	lines.push("Every tracked C Borg finding, indexed by the source site it names.");
	lines.push("This is the **index**, not a second authority: the issue board owns");
	lines.push("whether a finding is open, fixed or closed, and the JSON catalogue");
	lines.push("owns its provenance. What this table adds is the *reverse* lookup --");
	lines.push("from a function in `src/borg/` to everything known to be wrong with");
	lines.push("it -- which is what a rebuild needs when it rewrites that function.");
	lines.push("");
	lines.push(
		`All ${records.length} findings; ${files.length} source files; ` +
		`${records.reduce((n, r) => n + siteEntries(r).length, 0)} site entries.`
	);
	lines.push("");
	lines.push("| Source file | Function | Findings |");
	lines.push("| --- | --- | --- |");

	for (const file of files) {
		const fns = byFile.get(file);
		for (const fn of [...fns.keys()].sort((a, b) => a.localeCompare(b))) {
			const items = fns.get(fn).sort((a, b) => a.id.localeCompare(b.id));
			entries += items.length;
			const ids = items.map((r) => `${r.id} (#${issueNumber(r)})`).join(", ");
			lines.push(`| \`${file}\` | \`${fn}\` | ${ids} |`);
		}
	}

	lines.push("");
	lines.push(
		`**${entries} site entries over ${records.length} findings.** ` +
		"A finding whose site names two files appears under both, which is why " +
		"the entry count exceeds the finding count."
	);

	return { text: lines.join("\n") + "\n", entries };
}

/* ------------------------------------------------------------------ */

function buildHotspots(records) {
	const byFile = new Map();
	for (const record of records) {
		for (const { file } of siteEntries(record)) {
			if (!byFile.has(file)) byFile.set(file, new Set());
			byFile.get(file).add(record.id);
		}
	}

	const rows = [];
	for (const [file, ids] of byFile) {
		const recs = records.filter((r) => ids.has(r.id));
		rows.push({ file, count: recs.length, open: recs.length });
	}
	rows.sort((a, b) => b.count - a.count || a.file.localeCompare(b.file));

	const lines = [];
	lines.push("### 11.14 Where the defects concentrate");
	lines.push("");
	lines.push("Files in `src/borg/` ordered by how many findings name them. A file");
	lines.push("with many findings is either a hot spot in the code or a hot spot in");
	lines.push("the analysis; either way it is where a rebuild must be careful.");
	lines.push("");
	lines.push("| Rank | Source file | Findings |");
	lines.push("| ---: | --- | ---: |");

	rows.forEach((row, i) => {
		lines.push(`| ${i + 1} | \`${row.file}\` | ${row.count} |`);
	});

	const top = rows.slice(0, 5);
	lines.push("");
	lines.push(
		`**The top five files carry ${top.reduce((n, r) => n + r.count, 0)} of the ` +
		`${records.length} findings.** ` +
		top.map((r) => `\`${r.file.split("/").pop()}\` (${r.count})`).join(", ") + "."
	);
	lines.push("");
	lines.push("That distribution is itself a finding: the danger estimate, the");
	lines.push("home shopping, the kill flow, the debug commands and the caution");
	lines.push("phase are where the analysis has concentrated. A rebuild that");
	lines.push("touches those five has the most to get wrong.");

	return { text: lines.join("\n") + "\n", rows };
}

/* ------------------------------------------------------------------ */

const records = loadFindings();
const index = buildIndex(records);
const hotspots = buildHotspots(records);

const spec = readFileSync(SPEC, "utf8");

function section(text, startMark, endMark) {
	return (
		`<!-- ${startMark} -->\n` +
		text +
		`<!-- ${endMark} -->\n`
	);
}

const indexSection = section(index.text, "borg-defect-index:start", "borg-defect-index:end");
const hotspotsSection = section(hotspots.text, "borg-defect-hotspots:start", "borg-defect-hotspots:end");

function replaceMarked(body, startMark, endMark, replacement) {
	const start = `<!-- ${startMark} -->\n`;
	const end = `<!-- ${endMark} -->\n`;
	const i = body.indexOf(start);
	const j = body.indexOf(end);
	if (i === -1 || j === -1) {
		throw new Error(`markers ${startMark} / ${endMark} not found in the specification`);
	}
	return body.slice(0, i) + replacement + body.slice(j + end.length);
}

let updated = replaceMarked(spec, "borg-defect-index:start", "borg-defect-index:end", indexSection);
updated = replaceMarked(updated, "borg-defect-hotspots:start", "borg-defect-hotspots:end", hotspotsSection);

if (mode === "check") {
	if (updated !== spec) {
		console.error(
			"borg-defect-index: the specification's defect index does not match the catalogue."
		);
		console.error("Run: node tools/borg-defect-index.mjs --write");
		process.exit(1);
	}
	console.log(
		`borg-defect-index: ${records.length} findings, ${index.entries} site entries, ` +
		`${hotspots.rows.length} files; the specification matches the catalogue.`
	);
} else {
	writeFileSync(SPEC, updated);
	console.log(
		`borg-defect-index: wrote ${records.length} findings, ${index.entries} site entries, ` +
		`${hotspots.rows.length} files into the specification.`
	);
}
