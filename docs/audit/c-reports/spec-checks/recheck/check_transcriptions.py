#!/usr/bin/env python3
"""Check that every C code block in a Markdown document is a faithful
transcription of the pinned source.

Rules:
  * EVERY ```c block must be introduced, within the three non-blank lines
    above it, by a line that starts (after any indentation) with
    "Transcribed from" and names one or more `file:function:a-b` ranges.
    A ```c block without one is a failure (it would otherwise escape the
    check).  Pseudocode must use another fence label.
  * After removing comments and normalising whitespace, the block's lines
    must occur, in order, in the cited range(s).
  * Source lines skipped BETWEEN two matched block lines must be marked by a
    `...` line in the block at that point.  Lines before the first match and
    after the last match may be omitted only if the header says the block is
    an excerpt ("with fields omitted", "abridged", or a range that is only
    part of a function is fine: leading/trailing lines are not checked).

Usage: check_transcriptions.py <pin-root> <doc.md>
"""
import os
import re
import sys

pin, doc = sys.argv[1], sys.argv[2]
cite_re = re.compile(r"`([\w.-]+\.(?:c|h)):([A-Za-z_]\w*):(\d+)(?:-(\d+))?`")


def find(fn):
    for dp, _, fns in os.walk(os.path.join(pin, "src")):
        if fn in fns:
            return os.path.join(dp, fn)


def norm(s):
    s = re.sub(r"/\*.*?\*/", " ", s)
    s = re.sub(r"//.*", " ", s)
    return " ".join(s.split())


text = open(doc, encoding="utf-8").read().split("\n")
bad = 0
nblocks = 0
i = 0
while i < len(text):
    if not text[i].startswith("```c"):
        i += 1
        continue
    j = i  # fence line
    k = j + 1
    block = []
    while not text[k].startswith("```"):
        block.append(text[k])
        k += 1
    nblocks += 1
    # header: up to three non-blank lines above the fence, joined
    hdr_lines = []
    h = j - 1
    while h >= 0 and len(hdr_lines) < 3:
        if text[h].strip():
            hdr_lines.insert(0, text[h].strip())
        h -= 1
    hdr = " ".join(hdr_lines)
    if "Transcribed from" not in hdr or not cite_re.search(hdr[hdr.index("Transcribed from"):]):
        bad += 1
        print(f"NO-HEADER {os.path.basename(doc)}:{j+1}: ```c block without a 'Transcribed from `file:function:a-b`' line")
        i = k + 1
        continue
    hdr = hdr[hdr.index("Transcribed from"):]
    src = []
    for m in cite_re.finditer(hdr):
        f, a, b = m.group(1), int(m.group(3)), int(m.group(4) or m.group(3))
        lines = open(find(f), encoding="utf-8", errors="replace").read().split("\n")
        whole = "\n".join(lines[a - 1:b])
        whole = re.sub(r"/\*.*?\*/", " ", whole, flags=re.S)
        src += [norm(x) for x in whole.split("\n") if norm(x)]
    pos = None       # index in src of the last matched line
    gap_marked = False
    for bl in block:
        nb = norm(bl)
        if not nb:
            continue
        if nb == "...":
            gap_marked = True
            continue
        start = 0 if pos is None else pos + 1
        try:
            at = src.index(nb, start)
        except ValueError:
            bad += 1
            print(f"MISMATCH {os.path.basename(doc)}:{j+1}: {nb!r} not found (in order) in the cited range")
            continue
        if pos is not None and at > pos + 1 and not gap_marked:
            bad += 1
            print(f"UNMARKED-GAP {os.path.basename(doc)}:{j+1}: {at - pos - 1} source line(s) "
                  f"omitted before {nb!r} without a '...' line")
        pos = at
        gap_marked = False
    i = k + 1
print(f"# {nblocks} C blocks checked, {bad} problems")
sys.exit(1 if bad else 0)
