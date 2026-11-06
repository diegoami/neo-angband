#!/usr/bin/env python3
"""Replay the recorded before/after comparison in fresh temporary directories.

Run from a clone of diegoami/neo-angband-tracker. Requires Git, Python 3,
CMake, a C build toolchain and flock; native platform build dependencies apply.
All fixture files are extracted from the immutable original evidence commit.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

EVIDENCE = "3de1e3123915587483395dd40c4d2e75704e65f4"
BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
REMOTE = "https://github.com/diegoami/neo-angband-tracker.git"
EVIDENCE_PATH = "docs/audit/c-reports/CBUG-0002/"
FIXED_SOURCE_SHA256 = "fccea5be3bb31c1e3a8e2aecd4a209fc2877abc2845f64cf98cf343550a1e972"
# This is the complete current proposal: no diagnostic or tracking comment.
FIX = (
    'diff --git a/src/borg/borg.c b/src/borg/borg.c\n'
    '--- a/src/borg/borg.c\n'
    '+++ b/src/borg/borg.c\n'
    '@@ -309,7 +309,7 @@\n'
    ' \n'
    '     /* Flush messages */\n'
    '     borg_parse(NULL);\n'
    '-    borg.dont_react = false;\n'
    '+    borg_dont_react = false;\n'
    ' \n'
    '     /* Check for key on the queue */\n'
    '     borg_ch = borg_inkey(true);\n'
)


def main():
    for tool in ("git", "cmake", "flock"):
        if not shutil.which(tool):
            raise RuntimeError(f"Required tool not found: {tool}")
    repo = Path(subprocess.check_output(
        ["git", "rev-parse", "--show-toplevel"], text=True).strip())
    for commit in (EVIDENCE, BASE):
        exists = subprocess.run(["git", "cat-file", "-e", commit + "^{commit}"],
                                cwd=repo, stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)
        if exists.returncode:
            subprocess.run(["git", "fetch", "--no-tags", REMOTE, commit],
                           cwd=repo, check=True)

    root = Path(tempfile.mkdtemp(prefix="angband-cbug0002-", dir="/tmp"))
    evidence, source, build = root / "evidence", root / "source", root / "build"
    evidence.mkdir()
    (evidence / "profile").mkdir()
    print(f"Replay directory: {root}", flush=True)
    print("Original worktrees, profiles and previous evidence are preserved.", flush=True)

    def original(path):
        return subprocess.check_output(["git", "show", EVIDENCE + ":" + path], cwd=repo)

    for name in ("scenario.c", "run-modes.py", "observation.patch"):
        text = original(EVIDENCE_PATH + name).decode()
        text = text.replace("/tmp/native-c-14-evidence", str(evidence))
        text = text.replace("/tmp/native-c-14-tests", str(root / "test-user"))
        (evidence / name).write_text(text)
    (evidence / "profile" / "borg.txt").write_bytes(original(EVIDENCE_PATH + "borg.txt"))
    (evidence / "reaction-reset.patch").write_text(FIX)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE],
                   cwd=repo, check=True)
    subprocess.run(["git", "apply", str(evidence / "observation.patch")],
                   cwd=source, check=True)

    def run(label, args, cwd=repo):
        log = root / (label + ".log")
        print(f"{label}: waiting for shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result = subprocess.run(["flock", "/tmp/angband-validation.lock", *args],
                                    cwd=cwd, stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text()[-6000:], file=sys.stderr)
            raise RuntimeError(f"{label} failed ({result.returncode}); see {log}")
        print(f"{label}: complete", flush=True)
        return log

    run("configure", ["cmake", "-S", str(source), "-B", str(build),
                      "-DSUPPORT_BORG=ON", "-DSUPPORT_TEST_FRONTEND=ON",
                      "-DCMAKE_BUILD_TYPE=Debug"])
    totals = {}
    for phase in ("before", "after"):
        if phase == "after":
            subprocess.run(["git", "apply", str(evidence / "reaction-reset.patch")],
                           cwd=source, check=True)
            digest = hashlib.sha256((source / "src/borg/borg.c").read_bytes()).hexdigest()
            if digest != FIXED_SOURCE_SHA256:
                raise RuntimeError("Corrected C file differs from the verified proposal")
        run(phase + "-build", ["cmake", "--build", str(build), "--parallel", "2"])
        # The native test library resolves ./lib relative to its working directory.
        run(phase + "-scenarios", [sys.executable, str(evidence / "run-modes.py"),
                                  phase, str(build / "game/CBUG0002Observer")],
            cwd=build / "game")
        log = run(phase + "-alltests", ["cmake", "--build", str(build),
                                        "--target", "alltests", "--parallel", "2"])
        text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
        if "965/965" not in text or "Total: 5/5" not in text:
            raise RuntimeError(f"Expected native test totals not found in {log}")
        totals[phase] = {"unit_checks": "965/965", "frontend_tests": "5/5"}

    before = json.loads((evidence / "before-results.json").read_text())["modes"]
    after = json.loads((evidence / "after-results.json").read_text())["modes"]
    if before["knockout"]["passed"]:
        raise RuntimeError("The uncorrected knockout case unexpectedly passed")
    if not all(result["passed"] and result["exit"] == 0 for result in after.values()):
        raise RuntimeError("A corrected scenario did not pass")
    for mode in ("heavy-stun", "paralysis", "none"):
        if not before[mode]["passed"]:
            raise RuntimeError(f"Uncorrected control failed: {mode}")
        if (evidence / f"before-{mode}.log").read_bytes() != (evidence / f"after-{mode}.log").read_bytes():
            raise RuntimeError(f"Control log changed: {mode}")
    for phase, delta in (("before", 0), ("after", 1)):
        text = (evidence / f"{phase}-knockout.log").read_text()
        if f"KILL_REACTION delta={delta}" not in text or "OUTCOME deaths=1" not in text:
            raise RuntimeError(f"Expected completed kill/reaction result missing: {phase}")

    result = {"base": BASE, "fixture_commit": EVIDENCE,
              "fixed_source_sha256": FIXED_SOURCE_SHA256, "directory": str(root),
              "knockout_kill_reactions": {"before": 0, "after": 1},
              "identical_controls": ["heavy-stun", "paralysis", "none"],
              "native_tests": totals}
    (root / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print("VERIFIED: knockout fails before and passes after; kill reactions 0 -> 1.", flush=True)
    print("VERIFIED: three controls identical; both native gates 965/965 + 5/5.", flush=True)
    print(f"All logs and result.json: {root}", flush=True)
    print("The temporary source worktree is retained for inspection.", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Replay failed: {error}", file=sys.stderr)
        sys.exit(1)
