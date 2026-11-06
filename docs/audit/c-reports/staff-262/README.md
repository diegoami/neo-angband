# The staff that only pointed at one

A level fifty priest wears a ring carrying the staff-holiness
activation and faces three white harpies to its east.

**The staff-holiness activation floods the area with goodness,
striking every evil monster in sight — but the Borg valued it as a
single-target bolt: radius zero, one monster on a line.** The maneuver
never set the LOS-dispel radius its effect and damage already describe
(`PROJECT_LOS:DISP_EVIL`, the same convention the dispel prayers use).
With the LOS-dispel launch corrected (the stacked walk fix), the
radius change turns the valuation into the sight-line sum it should
be: **one harpy at 28 becomes the sight lines at 60**.

The Wonder half of the report is deliberately left as a bolt: the
wonder activation is an aimed random-effect device (`aim:1`), for
which a single-target valuation is the only sane model.

Control: a second ring carrying the lesser acid bolt — a genuine
single-target activation — is valued identically at 28 before and
after, with byte-identical logs: the fix changes only the
staff-holiness radius.

Note on stacking: this correction is one token stacked on the
LOS-dispel walk correction (issue326, PR327) it depends on — at the
pristine base the radius change alone would route the maneuver into
the walk that discards its blast. The evidence runner verifies the
ancestry and measures the bolt reading at the walk correction alone.

Consequence: the Borg's staff-of-holiness-style activations were valued
as single-target attacks. No death or survival claim is made.

Canonical report: [issue262](https://github.com/diegoami/neo-angband-tracker/issues/262).
Lead supplied by borg-rs; independently validated and reproduced
natively here. Source proposal [PR-SOURCE](https://github.com/diegoami/neo-angband-tracker/pull/SOURCE),
stacked on [PR327](https://github.com/diegoami/neo-angband-tracker/pull/327).

## Verification

### Reproduce and watch the comparison

From a clone of `diegoami/neo-angband-tracker`, copy these three commands:

```sh
git fetch https://github.com/diegoami/neo-angband-tracker.git task/staff-262-evidence
git show 4b7fd0c88cdb59dde0ed72e456a3480332af5d91:docs/audit/c-reports/staff-262/reproduce.py > /tmp/reproduce-staff-262.py
python3 /tmp/reproduce-staff-262.py --fixtures-ref 4b7fd0c88cdb59dde0ed72e456a3480332af5d91 --full-tests
```

The runner verifies the ancestry (the correction is stacked on the
walk correction), builds the bolt-radius and LOS-radius variants from
the walk-correction commit `53ef84c7`, runs two recorded situations
per variant, checks the fail-before and pass-after matrix and the
byte-identical control, records both full gates and writes an offline
`replay.html`.

Open [replay.html](replay.html) (download raw and open locally).
Choose **The staff that only pointed at one**: the bolt radius values
one harpy at 28; the LOS radius values the sight lines at 60.

Actual recorded output, bolt radius (`before-holy.log`):

```text
GEOMETRY player=(5,20) harpies=(10,20)(11,19)(11,21) temp_n=3
RATING holy=28
OUTCOME holy=28
```

LOS radius (`after-holy.log`): `RATING holy=60`. Control (`control`):
`RATING bolt2=28` in both variants, byte-identical logs.

Full native gates (both variants, `before-alltests.log` and
`after-alltests.log`): **965/965 unit checks and 5/5 frontend tests**,
no relevant skips, checked from actual run summaries.

### Source explanation

Walk correction: `53ef84c743768fdde9abaebc27bd8526b4423de9` (issue326).
Stacked correction: `fe34f38d0c998cf34ace02d09bf97774f38e65df`. Three
lines in `src/borg/borg-fight-attack.c`: the `BF_ACT_STAFF_HOLY` case
gains `rad = 10` with a comment, matching the effect's data
(`effect:PROJECT_LOS:DISP_EVIL`) and the convention every other
LOS-dispel maneuver uses.

```sh
git fetch --no-tags origin task/c-borg-staff-holy-los
git show 53ef84c743768fdde9abaebc27bd8526b4423de9:src/borg/borg-fight-attack.c | sed -n '/case BF_ACT_STAFF_HOLY/,+8p'
git diff 53ef84c743768fdde9abaebc27bd8526b4423de9 fe34f38d0c998cf34ace02d09bf97774f38e65df -- src/borg/borg-fight-attack.c
```

### Conditions and limits

Native Human Priest level fifty, depth thirty, seed one thousand, lit
constructed 40x40 arena, a worn ring whose activation is the table's
STAFF_HOLY entry (and one carrying ACID_BOLT2 as the control),
charged, three awake white harpies east of the priest. Stacked on the
LOS-dispel walk correction; the bolt-radius reading is measured at
that correction alone. The measurement calls the public maneuver
chooser with `borg_simulate` set. This isolates the radius token, not
full autonomous play. No death or survival claim.

Files in this directory: `scenario.c` (native observer), `run-cases.py`
(mode runner), `make_replay.py`/`replay.html`/`replay-data.json`
(offline browser demo), `reproduce.py` (pinned runner), `borg.txt`
(disposable profile configuration), `result.json`/`receipt.json`
(outcomes and hashes), and the captured phase logs. [Full
receipt](receipt.json).
