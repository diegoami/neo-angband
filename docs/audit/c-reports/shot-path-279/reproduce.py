#!/usr/bin/env python3
"""Compare caster-origin projectile paths, native escape costs and independent per-spell classification."""
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
FIX = "1bce07d8080b520589c292c793cc0b95ef9df182"
DIRECTORY = "docs/audit/c-reports/shot-path-279/"
SOURCE = "src/borg/borg-projection.c"
PHASES = ("before", "per-spell-only", "both", "after")
PER_SPELL = "b223c4f18622883549682f9f997b76fe1f06fc73"
CONTROLS = ("blocked", "ball-control", "direct-control", "boulder-blocked")

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
    for pin in (BASE, FIX, PER_SPELL):
        if subprocess.run(["git", "cat-file", "-e", pin+"^{commit}"], cwd=repo,
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
            subprocess.run(["git", "fetch", "--no-tags", "origin", pin], cwd=repo, check=True)
    original = subprocess.check_output(["git", "show", BASE+":"+SOURCE], cwd=repo).decode()
    fixed = subprocess.check_output(["git", "show", FIX+":"+SOURCE], cwd=repo).decode()
    a=original.index("bool borg_projectable_pure(");b=original.index("bool borg_projectable_dark(",a)
    if original[a:b].count("if (ag->kill)")!=1 or original[:a]+original[a:b].replace("if (ag->kill)","if (dist && ag->kill)",1)+original[b:]!=fixed:
        raise RuntimeError("Source correction is not exactly the caster-origin exclusion")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    danger_path="src/borg/borg-danger.c"
    danger=subprocess.check_output(["git","show",BASE+":"+danger_path],cwd=repo).decode()
    per_spell=subprocess.check_output(["git","show",PER_SPELL+":"+danger_path],cwd=repo).decode()
    expected=danger.replace("    bool bolt       = false;\n", "",1).replace("for (q = 0; q < kill->ranged_attack; q++) {", "for (q = 0; q < kill->ranged_attack; q++) {\n        bool bolt = false;",1)
    if expected!=per_spell:raise RuntimeError("Independent per-spell alternative differs from checked source")
    variants={"before":original,"per-spell-only":original,"both":fixed,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-shot-path-279-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(SHOTPATH279Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(SHOTPATH279Observer PRIVATE src src/tests)
target_compile_definitions(SHOTPATH279Observer PRIVATE ALLOW_BORG)
target_link_libraries(SHOTPATH279Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
        (source/danger_path).write_text(per_spell if phase in ("per-spell-only","both") else danger)
        run(phase+"-build", ["cmake", "--build", str(build), "--parallel", "2"])
        run(phase+"-scenarios", [sys.executable, str(evidence/"run-cases.py"), phase,
                               str(build/"game/SHOTPATH279Observer"), str(evidence)])
        rows[phase]=json.loads((evidence/(phase+"-results.json")).read_text())
        if args.full_tests and phase in ("before", "after"):
            log=run(phase+"-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    if not all(r["passed"] for r in rows["after"].values()):raise RuntimeError("Corrected case failed")
    for phase in ("before","per-spell-only"):
        if sum(r["passed"] for r in rows[phase].values())!=2:raise RuntimeError("Expected four caster-origin failures absent")
    if not all(r["passed"] for r in rows["both"].values()):raise RuntimeError("Combined diagnostic case failed")
    for mode,row in rows["after"].items():
        g=row["geometry"]
        if g["caster_clear"]!=g["expected_clear"] or g["player_clear"]!=g["expected_clear"] or not g["same_start"] or g["wall"] or g["through_wall"] or g["blocker_endpoint"]!=(-1 if g["expected_clear"] else 1):
            raise RuntimeError("Native geometry contract failed: "+mode)
    for mode in rows["before"]:
        a,b=rows["before"][mode]["geometry"],rows["after"][mode]["geometry"]
        if any(a[k]!=b[k] for k in ("player_clear","same_start","wall","through_wall","blocker_endpoint")):
            raise RuntimeError("Unchanged player/endpoint/terrain control changed")
    for mode in ("blocked","boulder-blocked"):
        if (evidence/f"before-{mode}.log").read_bytes()!=(evidence/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed actual-intermediate-monster control")
    for mode in CONTROLS:
        def extract(phase):
            lines=(evidence/f"{phase}-{mode}.log").read_text().splitlines()
            return [l for l in lines if l.startswith(("FRAME ","DECISION ","OUTCOME "))]
        if extract("before")!=extract("after"):raise RuntimeError("Changed native action/outcome control: "+mode)
    a,b=rows["before"]["clear-path"],rows["after"]["clear-path"]
    if a["hp_lost"]!=54 or b["hp_lost"] or b["scrolls_spent"]!=1:raise RuntimeError("Missing open-path native escape consequence")
    a,b=rows["before"]["boulder-control"],rows["after"]["boulder-control"]
    if a["hp_lost"]!=18 or b["hp_lost"]!=29 or b["mana_spent"]!=2:raise RuntimeError("Missing unfavorable hill-giant counterexample")
    if rows["after"]["boulder-blocked"]["hp_lost"]:raise RuntimeError("Native blocked boulder reached player")
    if (source/SOURCE).read_text()!=fixed or (source/danger_path).read_text()!=danger:
        raise RuntimeError("Disposable source not restored to the helper correction only")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,"cases":rows,"identical_native_outcome_controls":list(CONTROLS),"identical_full_logs":["blocked","boulder-blocked"],
             "full_tests":gates,"control_comparison":["before","after"],"fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence/"make_replay.py"),str(evidence)],check=True)
    print("VERIFIED: original2/6, helper correction6/6; player/endpoint/terrain controls unchanged; two full intermediate-monster control logs identical; four native outcome controls unchanged; unfavorable boulder counterexample retained.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
