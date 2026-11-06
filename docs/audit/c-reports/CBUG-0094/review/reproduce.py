#!/usr/bin/env python3
"""Run the native preparation comparison and produce an offline browser replay."""
import argparse
from pathlib import Path
import subprocess
import sys
import tempfile

DIRECTORY = "docs/audit/c-reports/CBUG-0094/"

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--fixtures-ref", default="FETCH_HEAD")
    parser.add_argument("--full-tests", action="store_true")
    parser.add_argument("--cmake-arg", action="append", default=[])
    args = parser.parse_args()
    repo = Path(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], text=True).strip())
    ref = subprocess.check_output(["git", "rev-parse", args.fixtures_ref], cwd=repo, text=True).strip()
    root = Path(tempfile.mkdtemp(prefix="angband-cbug0094-review-", dir="/tmp"))
    for name, path in (("native.py", DIRECTORY + "reproduce.py"),
                       ("make_replay.py", DIRECTORY + "review/make_replay.py")):
        (root / name).write_bytes(subprocess.check_output(["git", "show", ref + ":" + path], cwd=repo))
    command = [sys.executable, str(root / "native.py"), "--fixtures-ref", ref]
    if args.full_tests:
        command.append("--full-tests")
    command.extend("--cmake-arg=" + option for option in args.cmake_arg)
    evidence = None
    with (root / "native-run.log").open("w") as log:
        process = subprocess.Popen(command, cwd=repo, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        for line in process.stdout:
            print(line, end="", flush=True); log.write(line); log.flush()
            if line.startswith("Native evidence directory: "):
                evidence = Path(line.removeprefix("Native evidence directory: ").strip()) / "evidence"
        if process.wait():
            raise RuntimeError(f"Native run failed; see {root / 'native-run.log'}")
    if evidence is None or not (evidence.parent / "result.json").exists():
        raise RuntimeError("Native run did not produce its verified result")
    subprocess.run([sys.executable, str(root / "make_replay.py"), str(evidence)], cwd=repo, check=True)
    print("Choose 'Haste is chosen', then 'Start', 'Show first cast' and 'Show final result'.")
    print("Original: regeneration then haste, 369 mana left. Corrected: haste only, 389 left.")
    print("Choose 'Regeneration is chosen': original361 mana left, corrected381.")

if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.CalledProcessError) as error:
        print(f"Replay failed: {error}", file=sys.stderr)
        sys.exit(1)
