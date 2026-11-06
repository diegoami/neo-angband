#!/usr/bin/env python3
"""Compare the missile launch guard between the original and the chosen-index correction."""
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
FIX = "405e4fd4880e7bef27d6a6328765a904638c8257"
DIRECTORY = "docs/audit/c-reports/missile-254/"
SOURCE = "src/borg/borg-fight-attack.c"
PHASES = ("before", "after")
CONTROLS = ("live-shot", "simulated")

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
    if original.count("    /* Nothing to use */\n    if (b_n < 0)\n        return 0;\n")!=1:
        raise RuntimeError("Expected original launch guard missing")
    expected=original.replace("    /* Nothing to use */\n    if (b_n < 0)\n        return 0;\n",
        "    /* Nothing to use: no candidate missile was chosen, and b_n never goes\n"
        "     * negative once initialized to zero (candidates only raise it). */\n"
        "    if (b_k < 0)\n        return 0;\n", 1)
    if expected!=fixed:
        raise RuntimeError("Source correction is not exactly the chosen-index guard")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-missile-254-", dir="/tmp"))
    source = root/"source"
    evidence = {"normal": root/"evidence"/"normal", "asan": root/"evidence"/"asan"}
    for label, directory in evidence.items():
        (directory/"profile").mkdir(parents=True)
        for name, data in payload.items(): (directory/name).write_bytes(data)
        (directory/"profile/borg.txt").write_bytes(payload["borg.txt"])
    print(f"Native evidence directory: {root}", flush=True)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source/"CMakeLists.txt"
    cmake.write_text(cmake.read_text()+f'''
add_executable(M254Observer "{evidence['normal']/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(M254Observer PRIVATE src src/tests)
target_compile_definitions(M254Observer PRIVATE ALLOW_BORG)
target_link_libraries(M254Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root/'test-user'}")
''')
    test_script=source/"tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root/"test-user/Test")))
    build = {"normal": root/"build", "asan": root/"build-asan"}
    def run(label, command):
        log=root/(label+".log"); print(f"{label}: shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result=subprocess.run(["flock", "/tmp/angband-validation.lock", *command], cwd=repo,
                                  stdout=output, stderr=subprocess.STDOUT)
        return log, result.returncode
    configure_common=["-S", str(source), "-DSUPPORT_BORG=ON", "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug", *args.cmake_arg]
    log, code = run("configure", ["cmake", *configure_common, "-B", str(build["normal"])])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("configure failed")
    log, code = run("configure-asan", ["cmake", *configure_common, "-B", str(build["asan"]),
        "-DCMAKE_C_FLAGS=-fsanitize=address -fno-omit-frame-pointer -g",
        "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address"])
    if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError("asan configure failed")
    # Scenario runs disable ASan's strcmp interceptor and leak check: the pristine
    # base otherwise reports an unrelated strcmp overread while the Borg sorts
    # monster names during init (borg-util.c).  The tracked defect is a
    # compiler-instrumented out-of-bounds read and needs no interceptors.
    rows={}; gates={}
    for phase in PHASES:
        (source/SOURCE).write_text(variants[phase])
        for label in ("normal","asan"):
            log, code = run(f"{phase}-{label}-build", ["cmake", "--build", str(build[label]), "--parallel", "2"])
            if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase}-{label} build failed")
            log, code = run(f"{phase}-{label}-scenarios", [sys.executable, str(evidence[label]/"run-cases.py"), phase,
                                 str(build[label]/"game/M254Observer"), str(evidence[label])])
            if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase}-{label} scenario runner failed")
            rows[(phase,label)]=json.loads((evidence[label]/f"{phase}-results.json").read_text())
        if args.full_tests and phase in PHASES:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build["normal"]), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    for mode,row in rows[("after","normal")].items():
        if not row["passed"]: raise RuntimeError("Corrected case failed: "+mode)
    before=rows[("before","normal")]
    if before["no-target"]["passed"] or before["no-target"]["outcome"]["keys"]!=3 or before["no-target"]["outcome"]["return"]!=0:
        raise RuntimeError("Missing original junk-key firing in no-target case")
    if rows[("after","normal")]["no-target"]["outcome"]["keys"]!=0:
        raise RuntimeError("Corrected no-target case still queues keys")
    for mode in CONTROLS:
        if not before[mode]["passed"]: raise RuntimeError("Original control failed: "+mode)
        if (evidence["normal"]/f"before-{mode}.log").read_bytes()!=(evidence["normal"]/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control log: "+mode)
    asan_before=rows[("before","asan")]
    if asan_before["no-target"]["passed"] or not asan_before["no-target"]["oob_read"]:
        raise RuntimeError("Expected sanitizer out-of-bounds read missing in original no-target case")
    for mode in CONTROLS:
        if not asan_before[mode]["passed"] or asan_before[mode]["asan"]:
            raise RuntimeError("Original asan control changed: "+mode)
    for mode,row in rows[("after","asan")].items():
        if not row["passed"] or row["asan"]: raise RuntimeError("Corrected asan case failed: "+mode)
    if (source/SOURCE).read_text()!=fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,
             "cases":{f"{phase}-{label}": rows[(phase,label)] for phase in PHASES for label in ("normal","asan")},
             "identical_full_logs":list(CONTROLS),"full_tests":gates,
             "asan_no_target":"heap-buffer-overflow READ in borg-fight-attack.c",
             "fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence["normal"]/"make_replay.py"),str(evidence["normal"])],check=True)
    print("VERIFIED: original queues 3 keys from index -1 with an out-of-bounds read; corrected queues nothing; two control logs identical.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
