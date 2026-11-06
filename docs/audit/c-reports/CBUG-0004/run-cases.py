#!/usr/bin/env python3
"""Capture full native cold-resistance cases, checking outcomes not only exit0."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

MODES = ("acid-only", "cold-only", "neither", "both", "physical-acid-control", "physical-cold-control")

def main():
    phase, executable, output = sys.argv[1:]
    root = Path(output).resolve(); rows = {}
    for mode in MODES:
        env = dict(os.environ, CBUG0004_MODE=mode, CBUG0004_PROFILE=str(root / "profile"))
        result = subprocess.run([str(Path(executable).resolve()), "-v"], env=env,
                                cwd=Path(executable).resolve().parent, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=60)
        text = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout.decode())
        (root / f"{phase}-{mode}.log").write_text(text)
        outcome = re.search(r"OUTCOME (.*)", text)
        row = {"exit": result.returncode, "completed": bool(outcome),
               "passed": "native-c/CBUG-0004 finished: 1/1 passed" in text}
        if outcome:
            row.update({key: int(value) for key, value in re.findall(r"(\w+)=(\d+)", outcome[1])})
        danger = re.search(r"DANGER current=(\d+) escape_threshold=(\d+)", text)
        if danger: row.update(danger=int(danger[1]), threshold=int(danger[2]))
        rows[mode] = row; print(phase, mode, json.dumps(row), flush=True)
    (root / f"{phase}-results.json").write_text(json.dumps(rows, indent=2) + "\n")
    if not all(row["completed"] and row["exit"] == 0 for row in rows.values()):
        raise RuntimeError("A native case aborted before its outcome")

if __name__ == "__main__": main()
