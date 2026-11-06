#!/usr/bin/env python3
"""Compare the Dispel Life rating filter between the original and the living-monster correction."""
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
FIX = "6912b85cb7b902616ad786dfc20afe3b4cf28b10"
DIRECTORY = "docs/audit/c-reports/dispel-248/"
SOURCE = "src/borg/borg-fight-attack.c"
PHASES = ("before", "after")
CONTROLS = ("bolt",)

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
    if original.count("""    /*  Dispel life */
    case BORG_ATTACK_DRAIN_LIFE:
        if (!(rf_has(r_ptr->flags, RF_NONLIVING)))
            dam = 0;
        if (!(rf_has(r_ptr->flags, RF_UNDEAD)))
            dam = 0;
        break;
""")!=1:
        raise RuntimeError("Expected original Dispel Life filter missing")
    expected=original.replace("""    /*  Dispel life */
    case BORG_ATTACK_DRAIN_LIFE:
        if (!(rf_has(r_ptr->flags, RF_NONLIVING)))
            dam = 0;
        if (!(rf_has(r_ptr->flags, RF_UNDEAD)))
            dam = 0;
        break;
""", """    /*  Dispel life: the game's drain only hurts living monsters
     * (nonliving is undead or nonliving). */
    case BORG_ATTACK_DRAIN_LIFE:
        if (rf_has(r_ptr->flags, RF_NONLIVING)
            || rf_has(r_ptr->flags, RF_UNDEAD))
            dam = 0;
        break;
""", 1)
    if expected!=fixed:
        raise RuntimeError("Source correction is not exactly the living-monster filter")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-dispel-248-", dir="/tmp"))
    source, build, evidence = root/"source", root/"build", root/"evidence"
    (evidence/"profile").mkdir(parents=True)
    for name, data in payload.items(): (evidence/name).write_bytes(data)
    (evidence/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(D248Observer "{evidence/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(D248Observer PRIVATE src src/tests)
target_compile_definitions(D248Observer PRIVATE ALLOW_BORG)
target_link_libraries(D248Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
                               str(build/"game/D248Observer"), str(evidence)])
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
    if before["living"]["passed"] or before["living"]["rating"]["value"]!=-21:
        raise RuntimeError("Missing negative original rating in living case")
    if before["living"]["outcome"]["hp_lost"]<=0:
        raise RuntimeError("Native cast did not hurt the living giant")
    if rows["after"]["living"]["rating"]["value"]<=0:
        raise RuntimeError("Corrected living rating not positive")
    if not before["undead"]["passed"] or before["undead"]["outcome"]["hp_lost"]!=0:
        raise RuntimeError("Original undead control failed")
    if rows["after"]["undead"]["outcome"]["hp_lost"]!=0 or rows["after"]["undead"]["rating"]["value"]>0:
        raise RuntimeError("Corrected undead control changed decision")
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
    print("VERIFIED: original rates Dispel Life below nothing (-21) while the game removes 66 HP; corrected rates it 19; bolt control identical.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
