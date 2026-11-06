#!/usr/bin/env python3
"""Build both native C variants and generate an offline equipment-swap replay.

Run from the repository after fetching task/cbug-0066-evidence. The default
fixture reference FETCH_HEAD is the revision fetched by the setup command.
Use --full-tests to also run both native unit/frontend regression gates.
"""
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
FIXED_SOURCE_SHA256 = "96dd28c786e73ddf211a94f1d73b288225c182caa5ca68cdcd746c33d96dd40e"
DIRECTORY = "docs/audit/c-reports/CBUG-0066/expanded/"
FIX = (
    'diff --git a/src/borg/borg-item-wear.c b/src/borg/borg-item-wear.c\n'
    '--- a/src/borg/borg-item-wear.c\n'
    '+++ b/src/borg/borg-item-wear.c\n'
    '@@ -700,7 +700,7 @@\n'
    '     }\n'
    ' \n'
    '     /* Pass on the swap which yields the best result */\n'
    '-    if (b_p1 <= b_p2) {\n'
    '+    if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {\n'
    '         b_p  = b_p2;\n'
    '         swap = weapon_swap - 1;\n'
    '     } else {\n'
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--full-tests", action="store_true")
    args = parser.parse_args()
    for command in ("git", "cmake", "flock"):
        if not shutil.which(command):
            raise RuntimeError(f"Required command not found: {command}")
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()

    def read(path):
        return subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo)

    # Read the bundle before creating a worktree, so a missing fetch fails clearly.
    payload = {name: read(DIRECTORY + name) for name in
               ("scenario.c", "run-expanded.py", "make_replay.py")}
    configuration = read("docs/audit/c-reports/CBUG-0066/borg.txt")
    if subprocess.run(["git", "cat-file", "-e", BASE + "^{commit}"], cwd=repo,
                      stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode:
        subprocess.run(["git", "fetch", "--no-tags", "https://github.com/diegoami/neo-angband-tracker.git", BASE],
                       cwd=repo, check=True)

    root = Path(tempfile.mkdtemp(prefix="angband-cbug0066-", dir="/tmp"))
    source, build, evidence = root / "source", root / "build", root / "evidence"
    (evidence / "profile").mkdir(parents=True)
    print(f"Replay directory: {root}", flush=True)
    for name, data in payload.items():
        text = data.decode().replace("/tmp/native-c-66-expanded", str(evidence))
        (evidence / name).write_text(text)
    (evidence / "profile/borg.txt").write_bytes(configuration)
    (root / "fix.patch").write_text(FIX)
    subprocess.run(["git", "worktree", "add", "--detach", str(source), BASE], cwd=repo, check=True)
    cmake = source / "CMakeLists.txt"
    cmake.write_text(cmake.read_text() + f'''
add_executable(CBUG0066ExpandedObserver "{evidence / 'scenario.c'}"
    $<TARGET_OBJECTS:OurCoreLib> $<TARGET_OBJECTS:OurUnitTestLib>)
target_include_directories(CBUG0066ExpandedObserver PRIVATE src src/tests)
target_compile_definitions(CBUG0066ExpandedObserver PRIVATE ALLOW_BORG)
target_link_libraries(CBUG0066ExpandedObserver PRIVATE ${{ANGBAND_CORE_LINK_LIBRARIES}})
add_compile_definitions(PRIVATE_USER_PATH="{root / 'test-user'}")
''')
    test_script = source / "tests/run-test"
    test_script.write_text(test_script.read_text().replace("~/.angband/Test", str(root / "test-user/Test")))

    def run(label, command, cwd=repo):
        log = root / (label + ".log")
        print(f"{label}: waiting for shared validation lock; log {log}", flush=True)
        with log.open("w") as output:
            result = subprocess.run(["flock", "/tmp/angband-validation.lock", *command],
                                    cwd=cwd, stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text()[-6000:], file=sys.stderr)
            raise RuntimeError(f"{label} failed; see {log}")
        print(f"{label}: complete", flush=True)
        return log

    run("configure", ["cmake", "-S", str(source), "-B", str(build),
                      "-DSUPPORT_BORG=ON", "-DSUPPORT_TEST_FRONTEND=ON", "-DCMAKE_BUILD_TYPE=Debug"])
    full_tests = {}
    for phase in ("before", "after"):
        if phase == "after":
            subprocess.run(["git", "apply", str(root / "fix.patch")], cwd=source, check=True)
            digest = hashlib.sha256((source / "src/borg/borg-item-wear.c").read_bytes()).hexdigest()
            if digest != FIXED_SOURCE_SHA256:
                raise RuntimeError("Corrected source differs from the clean proposal")
        run(phase + "-build", ["cmake", "--build", str(build), "--parallel", "2"])
        run(phase + "-scenarios", [sys.executable, str(evidence / "run-expanded.py"), phase,
                                  str(build / "game/CBUG0066ExpandedObserver")], cwd=build / "game")
        if args.full_tests:
            log = run(phase + "-alltests", ["cmake", "--build", str(build), "--target", "alltests", "--parallel", "2"])
            text = re.sub(r"\x1b\[[0-9;]*m", "", log.read_text())
            if "965/965" not in text or "Total: 5/5" not in text:
                raise RuntimeError(f"Expected native regression totals missing from {log}")
            full_tests[phase] = {"unit_checks": "965/965", "frontend_tests": "5/5"}

    before = json.loads((evidence / "before-expanded.json").read_text())["cases"]
    after = json.loads((evidence / "after-expanded.json").read_text())["cases"]
    affected = ("armour-better", "weapon-better", "fire-armour", "electric-weapon", "both-helpful")
    controls = ("armour-only", "weapon-only", "tie", "no-benefit")
    for mode in affected:
        if before[mode]["passed"] or not before[mode]["completed"] or before[mode]["exit"]:
            raise RuntimeError(f"Expected complete baseline failure missing: {mode}")
        if after[mode].get("hp_lost", 10**9) >= before[mode]["hp_lost"]:
            raise RuntimeError(f"No measured damage improvement: {mode}")
    if not all(case["passed"] and case["exit"] == 0 for case in after.values()):
        raise RuntimeError("A corrected case failed")
    for mode in controls:
        if not before[mode]["passed"]:
            raise RuntimeError(f"Baseline control failed: {mode}")
        if (evidence / f"before-{mode}.log").read_bytes() != (evidence / f"after-{mode}.log").read_bytes():
            raise RuntimeError(f"Control output changed: {mode}")
    subprocess.run([sys.executable, str(evidence / "make_replay.py"), str(evidence)], check=True)
    summary = {"base": BASE, "fixtures_ref": ref, "fixed_source_sha256": FIXED_SOURCE_SHA256,
               "affected": {m: {"before_hp_lost": before[m]["hp_lost"], "after_hp_lost": after[m]["hp_lost"]} for m in affected},
               "identical_controls": list(controls), "native_full_tests": full_tests,
               "replay": str(evidence / "replay.html")}
    (root / "result.json").write_text(json.dumps(summary, indent=2) + "\n")
    print("VERIFIED: five native failures before, nine passes after, four identical controls.", flush=True)
    print(f"Open this file in a browser: {evidence / 'replay.html'}", flush=True)
    print(f"All native logs and result.json: {root}", flush=True)


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Replay failed: {error}", file=sys.stderr)
        sys.exit(1)
