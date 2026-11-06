"""Check the proposed simple reversal against the same native test cases.

Called by reproduce.py --compare-simple; only its disposable source is edited.
The verified guarded source and binary are restored before this exits.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

root=Path(sys.argv[1]).resolve()
source,build,evidence=root/'source',root/'build',root/'evidence'
path=source/'src/borg/borg-item-wear.c'
guarded=path.read_bytes()
expected='96dd28c786e73ddf211a94f1d73b288225c182caa5ca68cdcd746c33d96dd40e'
if hashlib.sha256(guarded).hexdigest()!=expected:
    raise SystemExit('The disposable source is not the verified guarded version')
condition='if (weapon_swap && (!armour_swap || b_p2 <= b_p1)) {'
assert guarded.decode().count(condition)==1


def run(label,args):
    log=root/(label+'.log')
    print(f'{label}: waiting for shared validation lock; log {log}',flush=True)
    with log.open('w') as out:
        r=subprocess.run(['flock','/tmp/angband-validation.lock',*args],cwd=build/'game',stdout=out,stderr=subprocess.STDOUT)
    if r.returncode: raise RuntimeError(f'{label} failed; see {log}')


try:
    path.write_text(guarded.decode().replace(condition,'if (b_p2 <= b_p1) {',1))
    run('simple-reversal-build',['cmake','--build',str(build),'--target','CBUG0066ExpandedObserver','--parallel','2'])
    run('simple-reversal-cases',[sys.executable,str(evidence/'run-expanded.py'),'simple-reversal',str(build/'game/CBUG0066ExpandedObserver')])
    cases=json.loads((evidence/'simple-reversal-expanded.json').read_text())['cases']
    failed={name for name,case in cases.items() if not case['passed']}
    if failed!={'armour-only','weapon-only'}:
        raise RuntimeError(f'Unexpected simple-reversal failures: {sorted(failed)}')
    if not all(case['completed'] and case['exit']==0 for case in cases.values()):
        raise RuntimeError('A comparison case crashed or did not finish')
    for name in failed:
        output=(evidence/f'simple-reversal-{name}.log').read_text()
        if 'pack_index=-1' not in output:
            raise RuntimeError(f'Expected absent-item selection missing: {name}')
    result={'comparison':'if (b_p2 <= b_p1)', 'passes':7,'failures':2,
            'failed_cases':sorted(failed),'failure_reason':'Both single-backup cases select pack_index=-1, an absent item.',
            'guarded_condition_passes':9,'cases':{name:case['passed'] for name,case in cases.items()}}
    (root/'simple-reversal-result.json').write_text(json.dumps(result,indent=2)+'\n')
    print('VERIFIED: simple reversal passes 7/9; armour-only and weapon-only fail with pack_index=-1.',flush=True)
finally:
    path.write_bytes(guarded)
    run('restore-guarded-build',['cmake','--build',str(build),'--target','CBUG0066ExpandedObserver','--parallel','2'])
