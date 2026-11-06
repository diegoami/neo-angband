#!/usr/bin/env python3
"""Compare mixed storm-giant attacks, escape consequences and a separate path correction."""
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
FIX = "b223c4f18622883549682f9f997b76fe1f06fc73"
DIRECTORY = "docs/audit/c-reports/CBUG-0048/"
SOURCE = "src/borg/borg-danger.c"
PHASES = ("before", "path-only", "both", "after")
CONTROLS = ("ball-control", "direct-control", "boulder-control")

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
    expected=original.replace("    bool bolt       = false;\n", "",1).replace("for (q = 0; q < kill->ranged_attack; q++) {", "for (q = 0; q < kill->ranged_attack; q++) {\n        bool bolt = false;",1)
    if expected!=fixed:raise RuntimeError("Source correction is not exactly the per-spell declaration")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    projection_path="src/borg/borg-projection.c"
    projection=subprocess.check_output(["git","show",BASE+":"+projection_path],cwd=repo).decode()
    a=projection.index("bool borg_projectable_pure(");b=projection.index("bool borg_projectable_dark(",a)
    if projection[a:b].count("if (ag->kill)")!=1:raise RuntimeError("Path alternative scope changed")
    path_fix=projection[:a]+projection[a:b].replace("if (ag->kill)","if (dist && ag->kill)",1)+projection[b:]
    variants={"before":original,"path-only":original,"both":fixed,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-cbug0048-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(CBUG0048Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(CBUG0048Observer PRIVATE src src/tests)
target_compile_definitions(CBUG0048Observer PRIVATE ALLOW_BORG)
target_link_libraries(CBUG0048Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
        (source/projection_path).write_text(path_fix if phase in ("path-only","both") else projection)
        run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/CBUG0048Observer"), str(evidence)])
        rows[phase]=json.loads((evidence/(phase+"-results.json")).read_text())
        if args.full_tests and phase in ("before", "after"):
            log=run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    if not all(r["passed"] for r in rows["after"].values()):raise RuntimeError("Corrected case failed")
    if sum(r["passed"] for r in rows["before"].values())!=3 or any(rows["before"][m]["passed"] for m in ("clear-path","blocked")):
        raise RuntimeError("Expected original failures missing")
    if rows["path-only"]["blocked"]["passed"] or not rows["path-only"]["clear-path"]["passed"]:
        raise RuntimeError("Separate path alternative did not isolate the remaining mixed-attack defect")
    if not all(r["passed"] for r in rows["both"].values()):raise RuntimeError("Combined diagnostic case failed")
    a,b=rows["before"]["clear-path"],rows["after"]["clear-path"]
    if a["escape"] or a["hp_lost"]!=54 or not b["escape"] or b["hp_lost"] or b["scrolls_spent"]!=1:
        raise RuntimeError("Missing native open-path escape consequence")
    a,b=rows["before"]["blocked"],rows["after"]["blocked"]
    if a["escape"] or a["hp_lost"]!=59 or not b["escape"] or b["hp_lost"]!=54 or b["scrolls_spent"]!=1:
        raise RuntimeError("Missing still-hit counterexample and scroll cost")
    for mode in CONTROLS:
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed primary before/after control: "+mode)
    if (source/SOURCE).read_text()!=fixed or (source/projection_path).read_text()!=projection:
        raise RuntimeError("Disposable source not restored to the primary correction only")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,"identical_controls":list(CONTROLS),
             "full_tests":gates,"control_comparison":["before","after"],"fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: original3/5, primary correction5/5; three complete before/after controls identical; path-only still misses blocked mixed attack; native cost counterexample retained.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
