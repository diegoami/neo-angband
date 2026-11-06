#!/usr/bin/env python3
"""Capture actual native ring attacks, including expected pre-fix failures."""
import json
import os
from pathlib import Path
import re
import subprocess
import sys

MODES = ("left-ready", "right-charging-left-ready", "right-control", "both-ready", "left-charging", "no-ring", "blind-control")

def main():
    phase, executable, output = sys.argv[1:]
    root = Path(output).resolve(); rows = {}
    for mode in MODES:
        env = dict(os.environ, CBUG0062_MODE=mode, CBUG0062_PROFILE=str(root / "profile"))
        result = subprocess.run([str(Path(executable).resolve()), "-v"], env=env,
                                cwd=Path(executable).resolve().parent, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT, timeout=60)
        text = re.sub(r"\x1b\[[0-9;]*m", "", result.stdout.decode())
        (root / f"{phase}-{mode}.log").write_text(text)
        outcome = re.search(r"OUTCOME (.*)", text)
        row = {"exit": result.returncode, "completed": bool(outcome),
               "passed": "native-c/CBUG-0062 finished: 1/1 passed" in text}
        if outcome: row.update({k: int(v) for k, v in re.findall(r"(\w+)=(-?\d+)", outcome[1])})
        rows[mode] = row; print(phase, mode, json.dumps(row), flush=True)
    (root / f"{phase}-results.json").write_text(json.dumps(rows, indent=2) + "\n")
    if not all(row["completed"] and row["exit"] == 0 for row in rows.values()):
        raise RuntimeError("A native case aborted before its outcome")

if __name__ == "__main__": main()
