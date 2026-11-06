import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
cases=('named-dragon','ogre-control','unseen-control','it-control','lower-it-control')
rows={}
for mode in cases:
 env=os.environ|{'CBUG0086_MODE':mode,'CBUG0086_PROFILE':str(root/'profile')}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=60,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 match=re.search(r'OUTCOME (.*)',raw)
 row={'exit':result.returncode,'completed':bool(match),'passed':'native-c/CBUG-0086 finished: 1/1 passed' in raw}
 if match:row.update({k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match[1])})
 rows[mode]=row
 print(mode,row)
 if result.returncode or not match:raise RuntimeError(f'Incomplete {mode}')
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
