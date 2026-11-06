#!/usr/bin/env python3
"""Every ```c block preceded by "Transcribed from `f:fn:a-b`" (and an optional
"and `g:fn2:c-d`") must consist of source lines that occur, in order, in the
cited range(s), after removing comments and normalising whitespace.  `...`
lines are skips.  Usage: check_transcriptions.py <pin-root> <doc.md>"""
import os, re, sys

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
n = 0
i = 0
while i < len(text):
    ln = text[i]
    if ln.startswith("Transcribed from"):
        hdr = ln
        j = i + 1
        while not text[j].startswith("```"):
            hdr += " " + text[j]
            j += 1
        if not text[j].startswith("```c"):
            i = j
            continue
        k = j + 1
        block = []
        while not text[k].startswith("```"):
            block.append(text[k])
            k += 1
        src = []
        for m in cite_re.finditer(hdr):
            f, a, b = m.group(1), int(m.group(3)), int(m.group(4) or m.group(3))
            lines = open(find(f), encoding="utf-8", errors="replace").read().split("\n")
            whole = "\n".join(lines[a - 1:b])
            whole = re.sub(r"/\*.*?\*/", " ", whole, flags=re.S)
            src += [norm(x) for x in whole.split("\n") if norm(x)]
        n += 1
        pos = 0
        for bl in block:
            nb = norm(bl)
            if not nb or nb == "...":
                continue
            try:
                pos = src.index(nb, pos) + 1
            except ValueError:
                bad += 1
                print(f"MISMATCH {os.path.basename(doc)}:{k} block at line {j+1}: {nb!r}")
        i = k
    i += 1
print(f"# {n} transcribed blocks checked, {bad} mismatching lines")
sys.exit(1 if bad else 0)
