#!/usr/bin/env python3
"""Compare the weapon slay traits between the original and the accumulating correction."""
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
FIX = "9ab78ae605ca4463268a669121d69b216285c2f1"
DIRECTORY = "docs/audit/c-reports/slay-239/"
SOURCE = "src/borg/borg-trait.c"
PHASES = ("before", "after")
CONTROLS = ("nonanimal",)

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
    anchor = """        /* various slays */
        borg.trait[BI_WS_ANIMAL] = item->slays[RF_ANIMAL];
        borg.trait[BI_WS_EVIL]   = item->slays[RF_EVIL];
        borg.trait[BI_WS_UNDEAD] = item->slays[RF_UNDEAD];
        borg.trait[BI_WS_DEMON]  = item->slays[RF_DEMON];
        borg.trait[BI_WS_ORC]    = item->slays[RF_ORC];
        borg.trait[BI_WS_TROLL]  = item->slays[RF_TROLL];
        borg.trait[BI_WS_GIANT]  = item->slays[RF_GIANT];
        borg.trait[BI_WS_DRAGON] = item->slays[RF_DRAGON];"""
    replacement = """        /* various slays: accumulate across slots.  Assigning them per
         * slot made the last occupied slot (usually the light or boots,
         * with no slays) erase the wielded weapon's slays. */
        borg.trait[BI_WS_ANIMAL] |= item->slays[RF_ANIMAL];
        borg.trait[BI_WS_EVIL]   |= item->slays[RF_EVIL];
        borg.trait[BI_WS_UNDEAD] |= item->slays[RF_UNDEAD];
        borg.trait[BI_WS_DEMON]  |= item->slays[RF_DEMON];
        borg.trait[BI_WS_ORC]    |= item->slays[RF_ORC];
        borg.trait[BI_WS_TROLL]  |= item->slays[RF_TROLL];
        borg.trait[BI_WS_GIANT]  |= item->slays[RF_GIANT];
        borg.trait[BI_WS_DRAGON] |= item->slays[RF_DRAGON];

        /* the greater, race-destroying slays are stored under the same
         * race flag with the largest multiplier */
        if (item->slays[RF_UNDEAD] >= 5)
            borg.trait[BI_WK_UNDEAD] = true;
        if (item->slays[RF_DEMON] >= 5)
            borg.trait[BI_WK_DEMON] = true;
        if (item->slays[RF_DRAGON] >= 5)
            borg.trait[BI_WK_DRAGON] = true;"""
    if original.count(anchor)!=1:
        raise RuntimeError("Expected original slay assignments missing")
    if original.replace(anchor, replacement, 1)!=fixed:
        raise RuntimeError("Source correction is not exactly the accumulating slays")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-slay-239-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(S239Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(S239Observer PRIVATE src src/tests)
target_compile_definitions(S239Observer PRIVATE ALLOW_BORG)
target_link_libraries(S239Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root/'test-user'}")
''')
    test_script=source/"tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root/"test-user/Test")))
    def run(label, command):
        log=root/(label+".log"); print(f"{label}: shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result=subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=repo,
                                  stdout=output, stderr=subprocess.STDOUT)
        return log, result.returncode
    log, code = run("configure", ["cmake", "-S", str(source), "-B", str(build), "-DSUPPORT_BORG=ON",
                      "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("configure failed")
    rows={}; gates={}
    for phase in PHASES:
        (source/SOURCE).write_text(variants[phase])
        log, code = run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} build failed")
        log, code = run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/S239Observer"), str(evidence)])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase]=json.loads((evidence/f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    def ratings(phase, mode): return rows[phase][mode]["rating"]
    if rows["before"]["animal"]["passed"]:
        raise RuntimeError("Original unexpectedly passed the animal case")
    if not rows["after"]["animal"]["passed"]:
        raise RuntimeError("Corrected animal case failed")
    for phase in PHASES:
        r = ratings(phase, "animal")
        if "with=20" not in r or "without=17" not in r:
            raise RuntimeError(f"Unexpected {phase} animal readings: {r}")
    if "auto=17" not in ratings("before","animal"):
        raise RuntimeError("Original animal case does not show the lost slay")
    if "auto=20" not in ratings("after","animal"):
        raise RuntimeError("Corrected animal case does not show the slay")
    if not rows["after"]["nonanimal"]["passed"]:
        raise RuntimeError("Corrected control failed")
    for mode in CONTROLS:
        if not rows["before"][mode]["passed"]:
            raise RuntimeError("Original control failed: "+mode)
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control log: "+mode)
    if (source/SOURCE).read_text()!=fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,
             "identical_full_logs":list(CONTROLS),"full_tests":gates,
             "fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: the original rates the Slay Animal weapon at 17 against an animal (the no-slay reading), the correction at 20; control identical.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
