#!/usr/bin/env python3
"""Compare real healing-potion use before, after and with a simple timing reversal."""
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
FIX = "cedf25eb790f2ff7c7c161cd1612b18ed99fb35e"
DIRECTORY = "docs/audit/c-reports/CBUG-0061/"
SOURCE = "src/borg/borg-item-use.c"
PHASES = ("before", "simple", "after")
AFFECTED = ("long-gap", "recent", "boundary-four", "first-use")
CONTROLS = ("same-turn", "bypass-control", "reserve-control", "empty-control", "first-turn")

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
    old = "if (borg_timer(when_last_quaff) > 4 && borg_timer(when_last_quaff) > 0"
    replacement = "if (when_last_quaff && borg_timer(when_last_quaff) <= 4\n        && borg_timer(when_last_quaff) > 0"
    if original.count(old)!=1 or original.replace(old,replacement)!=fixed:
        raise RuntimeError("Pinned correction differs from the checked timing condition")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source proposal scope changed")
    variants={"before":original,"simple":original.replace("borg_timer(when_last_quaff) > 4","borg_timer(when_last_quaff) <= 4",1),"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-cbug0061-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(CBUG0061Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(CBUG0061Observer PRIVATE src src/tests)
target_compile_definitions(CBUG0061Observer PRIVATE ALLOW_BORG)
target_link_libraries(CBUG0061Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
                               str(build/"game/CBUG0061Observer"), str(evidence)])
        rows[phase]=json.loads((evidence/(phase+"-results.json")).read_text())
        if args.full_tests and phase in ("before", "after"):
            log=run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    if not all(r["passed"] for r in rows["after"].values()):raise RuntimeError("Corrected case failed")
    if sum(r["passed"] for r in rows["before"].values())!=5 or any(rows["before"][m]["passed"] for m in AFFECTED):
        raise RuntimeError("Expected four baseline failures absent")
    if sum(r["passed"] for r in rows["simple"].values())!=8 or rows["simple"]["first-turn"]["passed"]:
        raise RuntimeError("Simple reversal did not retain first-drink regression")
    for mode in ("long-gap","first-use"):
        a,b=rows["before"][mode],rows["after"][mode]
        if a["used"] or a["healed"] or not a["confused"] or not b["used"] or b["healed"]!=60 or b["confused"] or b["potions_spent"]!=1:
            raise RuntimeError("Missing measured healing/confusion correction")
    for mode in ("recent","boundary-four"):
        a,b=rows["before"][mode],rows["after"][mode]
        if not a["used"] or a["healed"]!=60 or b["used"] or b["healed"] or not b["confused"]:
            raise RuntimeError("Missing recent-drink resource/health tradeoff")
    for mode in CONTROLS:
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control: "+mode)
    if (source/SOURCE).read_text()!=fixed:raise RuntimeError("Disposable source not restored")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,"identical_controls":list(CONTROLS),
             "full_tests":gates,"fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: original5/9, simple reversal8/9, complete fix9/9; five identical controls; recent-drink tradeoff retained.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
