# The message line that never ended

The screen row holds eighty characters, one of them two bytes wide, so
the text needs eighty-one bytes plus its terminator — one byte past the
wide-count limit the conversion used.

**`borg_what_text()` converted the wide screen text with a byte limit
of one per wide character, although a multibyte character occupies
more. A line whose byte length lands exactly on the limit was truncated
without its terminating zero — and the caller's buffer, uninitialized,
was then trimmed and measured as a string.** Measured with the buffer
junked and no zero anywhere: the original measurement runs through all
1024 stale bytes; the correction bounds the conversion by the widest
encoding for the same character count, so the full text converts and
terminates at 81.

Control: a pure-ASCII row fits the old limit as well — both variants
terminate identically at 80 with byte-identical logs.

Consequence: in play, any multibyte message line hitting the exact byte
boundary (UTF-8 names such as Carn Dûm) leaves the message text
unterminated, and the Borg parses stale stack bytes as message text —
the supplied harness evidence recorded a phantom monster from exactly
this. No death or survival claim is made.

Canonical report: [issue287](https://github.com/diegoami/neo-angband-tracker/issues/287).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/text-287-evidence
git show c3c980b3462773810c560931e3ae51b4996c9316:docs/audit/c-reports/text-287/reproduce.py > /tmp/reproduce-text-287.py
python3 /tmp/reproduce-text-287.py --fixtures-ref c3c980b3462773810c560931e3ae51b4996c9316 --full-tests
```

The runner builds original and corrected variants from pristine C
`e473720`, verifies the source diff is exactly the bounded conversion,
runs two recorded situations per variant, checks the fail-before and
pass-after matrix and the byte-identical control, records both full
gates and writes an offline `replay.html`.

Open [replay.html](replay.html) (download raw and open locally). Choose
**The message line that never ended**: the original reads 1024 stale
bytes; the correction terminates at 81, the full text.

Actual recorded output, original (`before-multibyte.log`):

```text
GEOMETRY rc=0 strnlen=1024 terminated=0
RATING terminated=0 strnlen=1024
OUTCOME terminated=0 strnlen=1024
```

Corrected (`after-multibyte.log`): `RATING terminated=1 strnlen=81`.
Control (`control`): `RATING terminated=1 strnlen=80` in both variants,
byte-identical logs.

Full native gates (original and corrected, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Original C: `e473720ed88a0f0c7ca66815cef3000eb4a38bc3`. Correction:
`3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1`. One block in
`src/borg/borg-io.c`. `wcstombs(s, screen_str, ABS(n) + 1)` limits
bytes while `n` counts wide characters; when the converted byte length
reaches the limit the result has no terminating zero. The correction
bounds the conversion by `ABS(n) * MB_CUR_MAX` — the widest encoding
for the same character count, still within the callers' 1024-byte
buffers for every reachable `n` — and falls back to an empty string on
a conversion error.

```sh
git fetch --no-tags origin task/c-borg-what-text-terminates
git show e473720ed88a0f0c7ca66815cef3000eb4a38bc3:src/borg/borg-io.c | sed -n '118,132p'
git diff e473720ed88a0f0c7ca66815cef3000eb4a38bc3 3e7fe8a8399352c4fe67f5de2d8d48b5e0bd04c1 -- src/borg/borg-io.c
```

### Conditions and limits

Native Human Warrior, seed one thousand, a UTF-8 locale (the multibyte
conversion only widens in a multibyte locale), a hand-painted screen
row of eighty characters with exactly one two-byte character, and a
caller buffer deliberately junked with no zero anywhere so an
unterminated conversion is visible as a full-buffer measurement. This
isolates the conversion bound, not the full message-parsing chain; the
phantom-monster consequence is the supplied harness evidence, not
claimed here. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
