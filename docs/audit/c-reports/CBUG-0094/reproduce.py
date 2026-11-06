#!/usr/bin/env python3
"""Reproduce the native C preparation bug from immutable source and fixture pins.

Run from a neo-angband-tracker clone containing the evidence commit.
Requires Python 3, Git, CMake, a C compiler/Make, flock and X11 development files.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
FIX = "31a042d00581ed914d30f17cca1ed0e70e99d21f"
DIRECTORY = "docs/audit/c-reports/CBUG-0094/"
AFFECTED = ("regen-only", "speed-wins")
CONTROLS = ("speed-only", "already-active", "unlearned", "blind", "confused", "town", "low-mana")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="HEAD")
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[], help="extra CMake configuration option")
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command):
            raise RuntimeError(f"Missing prerequisite: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()
    def read(path):
        return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)
    payload = {name: read(DIRECTORY + name) for name in ("scenario.c", "run-cases.py", "borg.txt")}
    for pin in (BASE, FIX):
        if subprocess.run(["git", "cat-file", "-e", pin + "^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    patch = subprocess.check_output(["git", "diff", BASE, FIX, "--", "src/borg/borg-fight-perm.c"], cwd=repo)
    if not patch or subprocess.check_output(["git", "diff", "--name-only", BASE, FIX], cwd=repo).decode().strip() != "src/borg/borg-fight-perm.c":
        raise RuntimeError("Source proposal scope changed")
    root = Path(tempfile.mkdtemp(prefix="angband-cbug0094-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    (evidence / "profile").mkdir(parents=True)
    for name, data in payload.items():
        (evidence / name).write_bytes(data)
    (evidence / "profile/borg.txt").write_bytes(payload["borg.txt"])
    (root / "fix.patch").write_bytes(patch)
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source / "CMakeLists.txt"
    cmake.write_text(cmake.read_text() + f'''
add_executable(CBUG0094Observer "{evidence / 'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(CBUG0094Observer PRIVATE src src/tests)
target_compile_definitions(CBUG0094Observer PRIVATE ALLOW_BORG)
target_link_libraries(CBUG0094Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root / 'test-user'}")
''')
    test_script = source / "tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root / "test-user/Test")))
    def run(label, command, cwd=repo):
        log = root / (label + ".log")
        print(f"{label}: shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result = subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=cwd,
                                    stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text()[-4000:], file=sys.stderr)
            raise RuntimeError(f"{label} failed (exit {result.returncode})")
        return log
    run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg])
    full_tests = {}
    for phase in ("before", "after"):
        if phase == "after":
            subprocess.run(["git", "apply", str(root / "fix.patch")], cwd=source, check=True)
            expected = subprocess.check_output(["git", "show", FIX + ":src/borg/borg-fight-perm.c"], cwd=repo)
            if (source / "src/borg/borg-fight-perm.c").read_bytes() != expected:
                raise RuntimeError("Corrected source differs from the proposal")
        run(phase + "-build", ["cmake", "--build", str(build), "--parallel", "2"])
        run(phase + "-scenarios", [sys.executable, str(evidence / "run-cases.py"), phase,
                                  str(build / "game/CBUG0094Observer"), str(evidence)])
        if args.full_tests:
            log = run(phase + "-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            full_tests[phase] = {"units": "965/965", "frontends": "5/5"}
    before = json.loads((evidence / "before-results.json").read_text())
    after = json.loads((evidence / "after-results.json").read_text())
    for mode in AFFECTED:
        if before[mode]["passed"] or not before[mode]["completed"] or before[mode]["exit"]:
            raise RuntimeError(f"Missing complete native baseline failure: {mode}")
        if before[mode]["mana_spent"] - after[mode]["mana_spent"] != 20:
            raise RuntimeError(f"Expected 20 extra mana in baseline: {mode}")
        if before[mode]["action_energy"] - after[mode]["action_energy"] != 100:
            raise RuntimeError(f"Expected one extra command cost in baseline: {mode}")
    for mode in CONTROLS:
        if not before[mode]["passed"] or (evidence / f"before-{mode}.log").read_bytes() != (evidence / f"after-{mode}.log").read_bytes():
            raise RuntimeError(f"Control changed or failed: {mode}")
    if not all(row["passed"] and row["exit"] == 0 for row in after.values()):
        raise RuntimeError("A corrected case failed")
    summary = {"base": BASE, "fix": FIX, "fixtures_ref": ref, "before": before, "after": after,
               "identical_controls": list(CONTROLS), "full_tests": full_tests,
               "fixed_source_sha256": hashlib.sha256((source / "src/borg/borg-fight-perm.c").read_bytes()).hexdigest()}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    print("VERIFIED: two baseline failures, nine corrected passes, seven identical controls.", flush=True)
    print(f"Logs and result.json: {root}", flush=True)

if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}", file=sys.stderr)
        sys.exit(1)
