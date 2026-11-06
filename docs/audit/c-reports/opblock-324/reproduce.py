#!/usr/bin/env python3
"""Compare formula parses between the original and the operator-array-releasing free."""
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
FIX = "02be7739db306897f2441562ee8d27dddc15d36c"
DIRECTORY = "docs/audit/c-reports/opblock-324/"
SOURCE = "src/borg/borg-formulas-calc.c"
PHASES = ("before", "after")
CONTROLS = ("control",)

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
    anchor = """    mem_free(operators);
    operators = NULL;"""
    replacement = """    /* Release the backing pointer array too: it is allocated separately
     * by borg_array_add() once the block has an operator. */
    mem_free(operators->items);
    mem_free(operators);
    operators = NULL;"""
    if original.count(anchor)!=1:
        raise RuntimeError("Expected original free tail missing")
    if original.replace(anchor, replacement, 1)!=fixed:
        raise RuntimeError("Source correction is not exactly the operator array release")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-opblock-324-", dir="/tmp"))
    source, build, build_asan, evidence = root/"source", root/"build", root/"build-asan", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(T324Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(T324Observer PRIVATE src src/tests)
target_compile_definitions(T324Observer PRIVATE ALLOW_BORG)
target_link_libraries(T324Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
    log, code = run("configure-asan", ["cmake", "-S", str(source), "-B", str(build_asan),
                      "-DSUPPORT_BORG=ON", "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug",
                      "-DCMAKE_C_FLAGS=-fsanitize=address -g",
                      "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address", *args.cmake_arg])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("asan configure failed")
    rows={}; gates={}
    for phase in PHASES:
        (source/SOURCE).write_text(variants[phase])
        log, code = run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} build failed")
        log, code = run(phase+"-build-asan", ["cmake", "--build", str(build_asan),
                        "--target", "T324Observer", "--parallel", "2"])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} asan build failed")
        log, code = run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/T324Observer"), str(build_asan/"game/T324Observer"),
                               str(evidence)])
        if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase} scenario runner failed")
        rows[phase]=json.loads((evidence/f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    for phase in PHASES:
        if not rows[phase]["operators"]["passed"] or "formula_value=7" not in rows[phase]["operators"]["rating"]:
            raise RuntimeError("Operator formula does not evaluate to 7 in "+phase)
    if rows["before"]["asan"]["operator_block_leaks"] < 1:
        raise RuntimeError("Original leak report lacks the operator backing array")
    if rows["after"]["asan"]["operator_block_leaks"] != 0:
        raise RuntimeError("Corrected leak report still loses the operator backing array")
    if "no leaks" not in rows["after"]["asan"]["summary"]:
        raise RuntimeError("Corrected variant still leaks on the operator parse: "+rows["after"]["asan"]["summary"])
    for mode in CONTROLS:
        if not rows["before"][mode]["passed"] or not rows["after"][mode]["passed"]:
            raise RuntimeError("Control not clean in both variants: "+mode)
        if "control_value=42" not in rows["after"][mode]["rating"] or "control_value=42" not in rows["before"][mode]["rating"]:
            raise RuntimeError("Control value changed: "+mode)
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control log: "+mode)
    if (evidence/"before-operators.log").read_bytes()!=(evidence/"after-operators.log").read_bytes():
        raise RuntimeError("Changed operator-parse functional log")
    if (source/SOURCE).read_text()!=fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,
             "identical_full_logs":["control","operators"],"full_tests":gates,
             "fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: both variants compute the formula identically, but the original loses the operator list's backing array (LeakSanitizer: 160 bytes in 2 allocations); the correction frees it (no leaks); control and functional logs byte-identical.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
