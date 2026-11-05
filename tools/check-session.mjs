#!/usr/bin/env node
import { readFileSync, existsSync } from "node:fs";
import { execFileSync } from "node:child_process";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
const root = dirname(dirname(fileURLToPath(import.meta.url)));
const c = JSON.parse(readFileSync(join(root, "session.json"), "utf8"));
if (c.schema !== "angband/session@1" || !["mandos", "port-fixes", "engine-and-c-fixes"].includes(c.role)) throw new Error("Invalid session role");
for (const file of ["AGENTS.md", "RULES.md", "README.md", "PLAN.md", "CLAUDE.md", "docs/SESSION.md"]) if (!existsSync(join(root, file))) throw new Error("Missing bootstrap file: " + file);
const remote = execFileSync("git", ["remote", "get-url", "origin"], { cwd: root, encoding: "utf8" }).trim().replace(/\.git$/, "");
if (!remote.endsWith("/" + c.repository) && !remote.endsWith(":" + c.repository)) throw new Error("Origin does not match the session repository: " + remote);
if (c.role === "port-fixes" && !existsSync(join(root,"src/controller.ts"))) throw new Error("Wrong port source tree");
if (c.role === "engine-and-c-fixes" && !existsSync(join(root,"packages/core")) && !existsSync(join(root,"src/borg"))) throw new Error("Wrong engine/C source tree");
console.log(c.repository + ": " + c.role + "; bootstrap files and origin verified");
