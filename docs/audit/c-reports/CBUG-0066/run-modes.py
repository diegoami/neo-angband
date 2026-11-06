import hashlib,json,os,pathlib,subprocess,sys
root=pathlib.Path(__file__).resolve().parent
phase=sys.argv[1];binary=pathlib.Path(sys.argv[2]).resolve()
rows={}
for mode in ('armour-better','weapon-better','armour-only','weapon-only','tie','no-benefit'):
    r=subprocess.run([str(binary),'-v'],cwd=binary.parent,env=dict(os.environ,CBUG0066_MODE=mode),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
    (root/f'{phase}-{mode}.log').write_bytes(r.stdout)
    rows[mode]={'exit':r.returncode,'passed':b'finished: 1/1 passed' in r.stdout,'sha256':hashlib.sha256(r.stdout).hexdigest()}
    print(phase,mode,rows[mode]['passed'],flush=True)
(root/f'{phase}-results.json').write_text(json.dumps({'binary_sha256':hashlib.sha256(binary.read_bytes()).hexdigest(),'modes':rows},indent=2)+'\n')
