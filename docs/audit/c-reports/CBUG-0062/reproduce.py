#!/usr/bin/env python3
"""Build native ring attacks before, after and with each plausible one-loop fix."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

BASE = "e473720ed88a0f0c7ca66815cef3000eb4a38bc3"
FIX = "46c14b2828ed751ff8be7ac67df93b28cdc15f87"
DIRECTORY = "docs/audit/c-reports/CBUG-0062/"
SOURCE = "src/borg/borg-item-use.c"
PHASES = ("before", "detection-only", "activation-only", "after")
AFFECTED = ("left-ready", "right-charging-left-ready")
CONTROLS = ("right-control", "both-ready", "left-charging", "no-ring", "blind-control")

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    for command in ("git", "cmake", "flock", "cc", "make"):
        if not shutil.which(command): raise RuntimeError(f"Missing prerequisite: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()
    def read(path): return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)
    payload = {name: read(DIRECTORY + name) for name in ("scenario.c", "run-cases.py", "make_replay.py", "borg.txt")}
    for pin in (BASE, FIX):
        if subprocess.run(["git", "cat-file", "-e", pin+"^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    original = subprocess.check_output(["git", "show", BASE+":"+SOURCE], cwd=repo).decode()
    fixed = subprocess.check_output(["git", "show", FIX+":"+SOURCE], cwd=repo).decode()
    old = "for (i = INVEN_RIGHT; i < INVEN_LEFT; i++)"
    new = "for (i = INVEN_RIGHT; i <= INVEN_LEFT; i++)"
    if original.count(old) != 2 or original.replace(old, new) != fixed:
        raise RuntimeError("The pinned fix is not exactly the two ring loops")
    if subprocess.check_output(["git", "diff", "--name-only", BASE, FIX], cwd=repo).decode().strip() != SOURCE:
        raise RuntimeError("Source proposal scope changed")
    first, last = original.rsplit(old, 1)
    variants = {"before": original, "detection-only": original.replace(old, new, 1),
                "activation-only": first+new+last, "after": fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-cbug0062-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(CBUG0062Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(CBUG0062Observer PRIVATE src src/tests)
target_compile_definitions(CBUG0062Observer PRIVATE ALLOW_BORG)
target_link_libraries(CBUG0062Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root/'test-user'}")
''')
    test_script=source/"tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root/"test-user/Test")))
    def run(label, command):
        log=root/(label+".log"); print(f"{label}: shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result=subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=repo,
                                  stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{label} failed")
        return log
    run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg])
    rows={}; gates={}
    for phase in PHASES:
        (source/SOURCE).write_text(variants[phase])
        run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/CBUG0062Observer"), str(evidence)])
        rows[phase]=json.loads((evidence/(phase+"-results.json")).read_text())
        if args.full_tests and phase in ("before", "after"):
            log=run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    for phase in PHASES[:-1]:
        for mode in AFFECTED:
            row=rows[phase][mode]
            if row["passed"] or not row["completed"] or row["exit"] or row["activated"] or row["damage"]:
                raise RuntimeError(f"Expected complete missing-activation failure absent: {phase}/{mode}")
    for mode in AFFECTED:
        row=rows["after"][mode]
        if not row["activated"] or not row["killed"] or row["enemy_after"] != 0:
            raise RuntimeError(f"Corrected left ring did not execute its native attack: {mode}")
    if not all(row["passed"] and row["exit"]==0 for row in rows["after"].values()):
        raise RuntimeError("A corrected case failed")
    for mode in CONTROLS:
        baseline=(evidence/f"before-{mode}.log").read_bytes()
        for phase in PHASES:
            if not rows[phase][mode]["passed"] or baseline!=(evidence/f"{phase}-{mode}.log").read_bytes():
                raise RuntimeError(f"Unchanged control failed or changed: {phase}/{mode}")
    if (source/SOURCE).read_text()!=fixed: raise RuntimeError("Disposable source not restored to complete fix")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,"identical_controls":list(CONTROLS),
             "full_tests":gates,"fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: original5/7, each one-loop alternative5/7, complete fix7/7; five identical controls.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
