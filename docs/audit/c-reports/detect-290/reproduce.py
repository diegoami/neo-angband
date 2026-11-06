#!/usr/bin/env python3
"""Compare queued detection marks between the original and the panel-bounds correction."""
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
FIX = "02fb00af6abdb165fc1b445c42dc85d2263a3cdd"
DIRECTORY = "docs/audit/c-reports/detect-290/"
SOURCE = "src/borg/borg-update.c"
PHASES = ("before", "after")
CONTROLS = ("fresh",)

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
    anchor = """    /* Extract panel */
    q_x = o_w_x / borg_panel_wid();
    q_y = o_w_y / borg_panel_hgt();

    /* Handle failure */
    if (borg_failure) {"""
    replacement = """    /* Extract panel.  The offsets are from the panel the Borg last saw:
     * a reaction queued before a level change can address panels outside
     * the detection arrays resized for the new cave, so drop the panel
     * marks of such stale reactions. */
    q_x = o_w_x / borg_panel_wid();
    q_y = o_w_y / borg_panel_hgt();

    bool detect_panel_ok = (q_x >= 0 && q_y >= 0
        && q_x + 1 <= borg.panels.x && q_y + 1 <= borg.panels.y);

    /* Handle failure */
    if (borg_failure) {"""
    expected = original.replace(anchor, replacement, 1)
    for tag in ('wall', 'trap', 'door', 'both', 'TDE', 'evil', 'obj'):
        expected = expected.replace(f'else if (prefix(str, "{tag}")) {{',
                                    f'else if (detect_panel_ok && prefix(str, "{tag}")) {{', 1)
    cackles_old = """        if (created_traps) {
            /* Remove the flag which tells borg that
             * Trap Detection was done here
             */
            borg_detect_trap[o_w_y / borg_panel_hgt() + 0]
                            [o_w_x / borg_panel_wid() + 0]
                = false;
            borg_detect_trap[o_w_y / borg_panel_hgt() + 0]
                            [o_w_x / borg_panel_wid() + 1]
                = false;
            borg_detect_trap[o_w_y / borg_panel_hgt() + 1]
                            [o_w_x / borg_panel_wid() + 0]
                = false;
            borg_detect_trap[o_w_y / borg_panel_hgt() + 1]
                            [o_w_x / borg_panel_wid() + 1]
                = false;"""
    cackles_new = """        if (created_traps) {
            /* Remove the flag which tells borg that
             * Trap Detection was done here.  The old offsets can address
             * a panel outside the arrays resized for a new cave. */
            int reset_y = o_w_y / borg_panel_hgt();
            int reset_x = o_w_x / borg_panel_wid();
            if (reset_y >= 0 && reset_x >= 0 && reset_y + 1 <= borg.panels.y
                && reset_x + 1 <= borg.panels.x) {
                borg_detect_trap[reset_y + 0][reset_x + 0] = false;
                borg_detect_trap[reset_y + 0][reset_x + 1] = false;
                borg_detect_trap[reset_y + 1][reset_x + 0] = false;
                borg_detect_trap[reset_y + 1][reset_x + 1] = false;
            }"""
    expected = expected.replace(cackles_old, cackles_new, 1)
    if expected != fixed:
        raise RuntimeError("Source correction is not exactly the panel-bounds guards")
    if subprocess.check_output(["git","diff","--name-only",BASE,FIX],cwd=repo).decode().strip()!=SOURCE:
        raise RuntimeError("Source scope changed")
    variants={"before":original,"after":fixed}
    root = Path(tempfile.mkdtemp(prefix="angband-detect-290-", dir="/tmp"))
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
add_executable(D290Observer "{evidence['normal']/'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(D290Observer PRIVATE src src/tests)
target_compile_definitions(D290Observer PRIVATE ALLOW_BORG)
target_link_libraries(D290Observer PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
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
    # compiler-instrumented out-of-bounds row-pointer read and needs no
    # interceptors.
    rows={}; gates={}
    for phase in PHASES:
        (source/SOURCE).write_text(variants[phase])
        for label in ("normal","asan"):
            log, code = run(f"{phase}-{label}-build", ["cmake", "--build", str(build[label]), "--parallel", "2"])
            if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase}-{label} build failed")
            log, code = run(f"{phase}-{label}-scenarios", [sys.executable, str(evidence[label]/"run-cases.py"), phase,
                                 str(build[label]/"game/D290Observer"), str(evidence[label])])
            if code: print(log.read_text()[-4000:], file=sys.stderr); raise RuntimeError(f"{phase}-{label} scenario runner failed")
            rows[(phase,label)]=json.loads((evidence[label]/f"{phase}-results.json").read_text())
        if args.full_tests:
            log, code = run(phase+"-alltests", ["cmake", "--build", str(build["normal"]), "--target", "alltests", "--parallel", "2"])
            text=re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if code or "Total: 965/965 passed (100.0%)" not in text or "Total: 5/5" not in text or "... Failed" in text:
                raise RuntimeError("Expected native regression totals missing or frontend failure")
            gates[phase]={"units": "965/965", "frontends": "5/5"}
    if rows[("before","normal")]["stale"]["passed"]:
        raise RuntimeError("Original stale case unexpectedly survived")
    if rows[("after","normal")]["stale"]["passed"] is False or "last_row_mark=0" not in rows[("after","normal")]["stale"]["rating"]:
        raise RuntimeError("Corrected stale case did not drop the mark")
    asan_before = rows[("before","asan")]["stale"]
    if not asan_before["asan"] or not asan_before["oob_row"]:
        raise RuntimeError("Expected sanitizer out-of-bounds row read missing in original stale case")
    if not rows[("after","asan")]["stale"]["passed"] or rows[("after","asan")]["stale"]["asan"]:
        raise RuntimeError("Corrected asan stale case not clean")
    for mode in CONTROLS:
        if not rows[("before","normal")][mode]["passed"] or not rows[("after","normal")][mode]["passed"]:
            raise RuntimeError("Control not clean in both variants: "+mode)
        if "last_row_mark=1" not in rows[("after","normal")][mode]["rating"]:
            raise RuntimeError("Corrected control lost the legitimate mark")
        if (evidence["normal"]/f"before-{mode}.log").read_bytes()!=(evidence["normal"]/f"after-{mode}.log").read_bytes():
            raise RuntimeError("Changed control log: "+mode)
    if (source/SOURCE).read_text()!=fixed:
        raise RuntimeError("Disposable source not restored to the correction")
    summary={"base":BASE,"fix":FIX,"fixtures_ref":ref,
             "cases":{"normal":rows[("before","normal")],"asan":rows[("before","asan")]},
             "after":{"normal":rows[("after","normal")],"asan":rows[("after","asan")]},
             "identical_full_logs":list(CONTROLS),"full_tests":gates,
             "fixed_source_sha256":hashlib.sha256((source/SOURCE).read_bytes()).hexdigest()}
    (root/"result.json").write_text(json.dumps(summary,indent=2)+"\n")
    subprocess.run([sys.executable,str(evidence["normal"]/"make_replay.py"),str(evidence["normal"])],check=True)
    print("VERIFIED: the original crashes on the stale queued reaction (the sanitizer names the row-pointer read past the arrays in borg_handle_self); the correction drops it and completes; fresh control identical.",flush=True)
    print(f"Logs and result.json: {root}",flush=True)

if __name__=="__main__":
    try: main()
    except (OSError,RuntimeError,subprocess.CalledProcessError) as error:
        print(f"Reproduction failed: {error}",file=sys.stderr);sys.exit(1)
