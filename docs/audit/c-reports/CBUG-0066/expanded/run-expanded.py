import hashlib,json,os,pathlib,re,subprocess,sys
root=pathlib.Path(__file__).resolve().parent
phase=sys.argv[1];binary=pathlib.Path(sys.argv[2]).resolve()
modes=['armour-better','weapon-better','armour-only','weapon-only','tie','no-benefit','fire-armour','electric-weapon','both-helpful']
rows={}
for mode in modes:
    run=subprocess.run([str(binary),'-v'],cwd=binary.parent,env=dict(os.environ,CBUG0066_MODE=mode),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
    output=run.stdout.decode();(root/f'{phase}-{mode}.log').write_text(output)
    frames=[json.loads(line[6:]) for line in output.splitlines() if line.startswith('FRAME ')]
    hp=re.search(r'OUTCOME hp_before=(\d+) hp_after=(-?\d+) hp_lost=(\d+)',output)
    row={'exit':run.returncode,'passed':'finished: 1/1 passed' in output,'completed':'finished:' in output,'frames':frames,'engine_messages':[s[8:] for s in output.splitlines() if s.startswith('ENGINE: ')], 'sha256':hashlib.sha256(run.stdout).hexdigest()}
    if hp: row.update(dict(zip(['hp_before','hp_after','hp_lost'],map(int,hp.groups()))))
    rows[mode]=row
    print(phase,mode,'pass='+str(row['passed']),'damage='+str(row.get('hp_lost')),'frames='+str(len(frames)),flush=True)
(root/f'{phase}-expanded.json').write_text(json.dumps({'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'cases':rows},indent=2)+'\n')
