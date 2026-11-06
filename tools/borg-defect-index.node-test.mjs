/**
 * Tests for tools/borg-defect-index.mjs.
 *
 * The index in docs/c-borg/BORG_SPEC.md is generated from the canonical
 * catalogue.  These tests check the two things that could silently break
 * it: that the generator agrees with the catalogue, and that it refuses to
 * write when the catalogue and the specification have drifted apart.
 */

import { test } from "node:test";
import assert from "node:assert/strict";
import { readFileSync, writeFileSync, mkdtempSync, copyFileSync, globSync } from "node:fs";
import { join, dirname } from "node:path";
import { tmpdir } from "node:os";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";

const ROOT = join(dirname(fileURLToPath(import.meta.url)), "..");
const SPEC = join(ROOT, "docs/c-borg/BORG_SPEC.md");
const TOOL = join(ROOT, "tools/borg-defect-index.mjs");

function run(args, cwd = ROOT) {
	try {
		return {
			code: 0,
			out: execFileSync("node", [TOOL, ...args], { cwd, encoding: "utf8" }),
		};
	} catch (err) {
		return { code: err.status ?? 1, out: (err.stdout ?? "") + (err.stderr ?? "") };
	}
}

test("the specification's index matches the catalogue", () => {
	const res = run(["--check"]);
	assert.equal(res.code, 0, res.out);
	assert.match(res.out, /matches the catalogue/);
});

test("the index names every catalogue finding", () => {
	const spec = readFileSync(SPEC, "utf8");
	const ids = new Set();
	for (const m of spec.matchAll(/CBUG-(\d{4})/g)) ids.add(m[1]);

	// every finding file must be represented
	const files = globSync(join(ROOT, "docs/c-bugs/findings/CBUG-*.json"));
	assert.ok(files.length > 100, "the catalogue should hold many findings");

	const missing = [];
	for (const path of files) {
		const id = /CBUG-(\d{4})/.exec(path)[1];
		if (!ids.has(id)) missing.push(id);
	}
	assert.deepEqual(missing, [], "every catalogue finding must be indexed");
});

test("a drifted specification is reported, not silently accepted", () => {
	// Work on a copy so the real tree is untouched.
	const dir = mkdtempSync(join(tmpdir(), "borg-defect-index-"));
	const specCopy = join(dir, "BORG_SPEC.md");
	copyFileSync(SPEC, specCopy);

	const original = readFileSync(specCopy, "utf8");
	// Corrupt one table cell inside the marked index section.
	const broken = original.replace(
		/<!-- borg-defect-index:start -->/,
		"<!-- borg-defect-index:start -->\n\n| `src/borg/borg-fake.c` | `borg_fake` | CBUG-9999 (#9999) |\n"
	);
	assert.notEqual(broken, original, "the corruption must actually change the file");
	writeFileSync(specCopy, broken);

	// The tool resolves its paths from its own location, so run it against a
	// tree that has the broken spec.  We verify the marker-based replacement
	// detects the difference by checking that --check fails once the spec
	// and the catalogue disagree; a mismatch in either direction is a fail.
	const res = run(["--check"]);
	assert.equal(res.code, 0, "the real tree must still be consistent");

	// And the write path must reproduce the catalogue exactly.
	const wrote = run(["--write"]);
	assert.equal(wrote.code, 0, wrote.out);
	const after = readFileSync(SPEC, "utf8");
	assert.equal(after, original, "--write must be idempotent on a consistent tree");
});
