#!/usr/bin/env python3
"""Compare the out-of-light entry gate between the original and the dying-light correction."""
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
FIX = "54a26b3b3fe4e12ed4d8fc3380219727b48eda2b"
DIRECTORY = "docs/audit/c-reports/outoflight-229/"
SOURCE = "src/borg/borg-think-dungeon-util.c"
PHASES = ("before", "after")
CONTROLS = ("lit", "everburn", "dead")
OLD_GATE = (
    "    if (!borg.trait[BI_LIGHT]\n"
    "        && (borg.trait[BI_LIGHT] <= 0 || borg_items[INVEN_LIGHT].timeout <= 3)\n"
    "        && borg.trait[BI_CDEPTH] >= 1) {\n"
)
NEW_GATE = (
    "    if ((!borg.trait[BI_LIGHT]\n"
    "        || (borg_items[INVEN_LIGHT].timeout <= 3\n"
    "            && !of_has(borg_items[INVEN_LIGHT].flags, OF_NO_FUEL)))\n"
    "        && borg.trait[BI_CDEPTH] >= 1) {\n"
)

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
    if original.count(OLD_GATE) != 1:
        raise RuntimeError("Expected original self-canceling light gate missing")
    expected = original.replace(OLD_GATE, NEW_GATE, 1)
    if expected != fixed:
        raise RuntimeError("Source correction is not exactly the dying-light gate")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-outoflight-229-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(OL229Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(OL229Observer PRIVATE src src/tests)
target_compile_definitions(OL229Observer PRIVATE ALLOW_BORG)
target_link_libraries(OL229Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
                               str(build/"game/OL229Observer"), str(evidence)])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase]=json.loads((evidence/f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    for mode,row in rows["after"].items():
        if not row["passed"]: raise RuntimeError("Corrected case failed: "+mode)
    before=rows["before"]
    if before["dying"]["passed"]:
        raise RuntimeError("Original dying case unexpectedly passed: the fixture no longer demonstrates the dead clause")
    if before["dying"]["outcome"]["acted"]!=0:
        raise RuntimeError("Original dying case does not leave the dying light alone")
    if rows["after"]["dying"]["outcome"]["acted"]!=1 or rows["after"]["dying"]["outcome"]["keys"]==0:
        raise RuntimeError("Corrected dying case does not start the maneuvers")
    for mode in CONTROLS:
        if not before[mode]["passed"]: raise RuntimeError("Original control failed: "+mode)
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control log: "+mode)
    if (source/SOURCE).read_text()!=fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,
             "identical_full_logs":list(CONTROLS),"full_tests":gates,
             "fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: the original leaves a dying lantern alone, the correction starts the maneuvers and swaps to the torch; three control logs identical.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
