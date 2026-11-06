#!/usr/bin/env python3
"""Check a Markdown spec against a pinned Angband source tree.

1. Identifiers: every C-like token inside backticks or fenced code blocks
   must occur as a whole word somewhere under <pin>/src or <pin>/lib, or be a
   file that exists there.  Tokens in an ALLOW list (English words used in
   pseudocode, shell, numbers) are skipped and reported separately.
2. Citations of the form  file.c:function:N  or  file.c:function:N-M  must
   name a function defined in that file, and N..M must lie inside its body
   (signature line through closing brace).  For .h/.txt files and for
   non-function anchors, the anchor token must occur within N..M.

Usage: check_spec.py <pin-root> <doc.md> [more docs...]
Exit 1 on any miss.
"""
import os
import re
import sys

pin = sys.argv[1]
docs = sys.argv[2:]
REPO = os.environ.get("SPEC_REPO", ".")
REPO_DIRS = ["docs/audit/c-reports/spec-checks", "docs/c-borg", "docs/c-bugs"]
# files the Borg writes or reads at run time (not in the source tree)
RUNTIME_FILES = {"borg-log.txt", "borg.dat", "borg.txt"}

# ---------------------------------------------------------------- corpus
corpus_files = {}
words = set()
for top in ("src", "lib"):
    for dp, _, fns in os.walk(os.path.join(pin, top)):
        for fn in fns:
            p = os.path.join(dp, fn)
            try:
                text = open(p, encoding="utf-8", errors="replace").read()
            except OSError:
                continue
            rel = os.path.relpath(p, pin)
            corpus_files[fn] = corpus_files.get(fn, []) + [rel]
            if fn.endswith((".c", ".h", ".txt")):
                words.update(re.findall(r"[A-Za-z_][A-Za-z0-9_]*", text))

def src_lines(fn):
    for rel in corpus_files.get(fn, []):
        return open(os.path.join(pin, rel), encoding="utf-8", errors="replace").read().split("\n")
    return None

# ---------------------------------------------------------------- function bodies
def _clean(ln):
    ln = re.sub(r"'(?:[^'\\]|\\.)'", "''", ln)
    ln = re.sub(r'"(?:[^"\\]|\\.)*"', '""', ln)
    ln = re.sub(r"/\*.*?\*/", " ", ln)
    return re.sub(r"//.*", " ", ln)

def func_range(lines, name):
    """(start, end) 1-based of the definition of `name`, or None."""
    lines = [_clean(l) for l in lines]
    pat = re.compile(r"^(?:static\s+)?(?:const\s+)?[A-Za-z_][\w\s\*]*?\b" + re.escape(name) + r"\s*\(")
    for i, ln in enumerate(lines):
        if not pat.match(ln):
            continue
        # find opening brace before a semicolon
        j = i
        while j < len(lines) and j < i + 10:
            b = lines[j].find("{")
            s = lines[j].find(";")
            if s != -1 and (b == -1 or s < b):
                break
            if b != -1:
                depth = 0
                for k in range(j, len(lines)):
                    depth += lines[k].count("{") - lines[k].count("}")
                    if depth == 0 and k >= j:
                        return (i + 1, k + 1)
                return None
            j += 1
    return None

# ---------------------------------------------------------------- doc scanning
ALLOW = set("""
bss json md
a an and or not the of to in on at is it if for by be as do no yes y n
true false NULL int bool char void const static struct enum return else while
break continue sizeof long unsigned uint8_t uint16_t uint32_t int16_t int32_t
sh cd git gh gcc nm objdump node grep for done echo printf jq python3 flock
cmake make rev parse diff show log head tail sed awk wc sort uniq cut
mkdir find ls cat tar archive build api repos commits master main
O0 O2 Wall Wextra RelWithDebInfo DNDEBUG g
""".split())

failures = []
skipped = set()

def check_token(tok, where):
    if tok in ALLOW or re.fullmatch(r"\d+[A-Za-z]*|0x[0-9a-fA-F]+|[0-9a-f]{7,40}", tok):
        skipped.add(tok)
        return
    if tok in words:
        return
    # a prefix family written as `BF_*` / `borg_flow_*`
    if tok.endswith("_") and any(w.startswith(tok) for w in words):
        return
    failures.append(("IDENT", where, tok))

span_re = re.compile(r"`([^`\n]+)`")
cite_re = re.compile(r"\b([A-Za-z][\w.-]*\.(?:c|h|txt)):([A-Za-z_][A-Za-z0-9_]*):(\d+)(?:[-–](\d+))?")
file_re = re.compile(r"^(?:src/|lib/gamedata/|src/borg/)?([A-Za-z][\w.-]*\.(?:c|h|txt))$")
ident_re = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")

ncites = 0
nidents = 0
for doc in docs:
    text = open(doc, encoding="utf-8").read()
    lines = text.split("\n")
    in_fence = False
    fence_lang = ""
    for lno, ln in enumerate(lines, 1):
        where = f"{os.path.basename(doc)}:{lno}"
        if ln.startswith("```"):
            in_fence = not in_fence
            fence_lang = ln[3:].strip()
            continue
        # citations anywhere on the line
        for m in cite_re.finditer(ln):
            ncites += 1
            fn, anchor, l1, l2 = m.group(1), m.group(2), int(m.group(3)), int(m.group(4) or m.group(3))
            sl = src_lines(fn)
            if sl is None:
                failures.append(("CITE-FILE", where, m.group(0)))
                continue
            if l1 > l2 or l2 > len(sl):
                failures.append(("CITE-RANGE", where, f"{m.group(0)} (file has {len(sl)} lines)"))
                continue
            rng = func_range(sl, anchor) if fn.endswith(".c") else None
            if rng:
                if not (rng[0] <= l1 and l2 <= rng[1]):
                    failures.append(("CITE-BODY", where, f"{m.group(0)} but {anchor} is {rng[0]}-{rng[1]}"))
            else:
                seg = "\n".join(sl[l1 - 1:l2])
                if not re.search(r"\b" + re.escape(anchor) + r"\b", seg):
                    failures.append(("CITE-ANCHOR", where, f"{m.group(0)}: {anchor} not in lines"))
        if in_fence:
            if fence_lang in ("sh", "console", "text", "output"):
                continue
            if fence_lang == "pseudocode":
                # only identifier-shaped tokens: contain '_' , or are called, or are members
                for mm in re.finditer(r"[A-Za-z_][A-Za-z0-9_]*(?:\(|\[|\.|->)?", ln):
                    t = mm.group(0)
                    base = re.match(r"[A-Za-z_][A-Za-z0-9_]*", t).group(0)
                    if "_" in base.strip("_") or t != base:
                        nidents += 1
                        check_token(base, where)
                continue
            # strip comments and string literals in code blocks
            code = re.sub(r"/\*.*?\*/", " ", ln)
            code = re.sub(r"//.*", " ", code)
            code = re.sub(r"--.*", " ", code)
            code = re.sub(r'"(?:[^"\\]|\\.)*"', " ", code)
            code = re.sub(r"'(?:[^'\\]|\\.)*'", " ", code)
            for tok in ident_re.findall(code):
                nidents += 1
                check_token(tok, where)
            continue
        for sm in span_re.finditer(ln):
            span = sm.group(1).strip()
            if cite_re.search(span):
                continue
            fm = file_re.match(span)
            if fm:
                if (fm.group(1) not in corpus_files and span not in RUNTIME_FILES
                        and not any(os.path.exists(os.path.join(REPO, d, span)) for d in REPO_DIRS)):
                    failures.append(("FILE", where, span))
                continue
            if re.fullmatch(r"[\w./-]+/", span) or span.startswith(("docs/", "tools/", "patches/", "node ", "gh ", "git ", "nm ", "gcc ", "cmake ", "angband/", "origin/", "http", "-O", "-D")) or span in RUNTIME_FILES or os.path.exists(os.path.join(REPO, span)) or any(os.path.exists(os.path.join(REPO, d, span)) for d in REPO_DIRS):
                continue
            code = re.sub(r'"(?:[^"\\]|\\.)*"', " ", span)
            code = re.sub(r"'(?:[^'\\]|\\.)*'", " ", code)
            for tok in ident_re.findall(code):
                nidents += 1
                check_token(tok, where)

for kind, where, what in failures:
    print(f"{kind}\t{where}\t{what}")
print(f"# {ncites} file:function:line citations, {nidents} identifier tokens, "
      f"{len(failures)} failures; skipped (allow-list) tokens: {' '.join(sorted(skipped))}")
sys.exit(1 if failures else 0)
