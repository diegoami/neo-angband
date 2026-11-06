import hashlib,json,os,pathlib,subprocess,sys
root=pathlib.Path(__file__).resolve().parent
phase=sys.argv[1]
binary=sys.argv[2]
results={}
for mode in ('knockout','heavy-stun','paralysis','none'):
    env=dict(os.environ,CBUG0002_MODE=mode)
    run=subprocess.run([binary,'-v'],env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=60)
    (root/f'{phase}-{mode}.log').write_bytes(run.stdout)
    results[mode]={'passed':b'finished: 1/1 passed' in run.stdout,'exit':run.returncode,'output_sha256':hashlib.sha256(run.stdout).hexdigest()}
    print(phase,mode,results[mode],flush=True)
(root/f'{phase}-results.json').write_text(json.dumps({'binary_sha256':hashlib.sha256(pathlib.Path(binary).read_bytes()).hexdigest(),'modes':results},indent=2)+'\n')
