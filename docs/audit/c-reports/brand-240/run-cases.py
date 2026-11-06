import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
cases=('brand','control')
rows={}
for mode in cases:
 env=os.environ|{'N240_MODE':mode,'N240_PROFILE':str(root/'profile')}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/brand-240 finished: 1/1 passed' in raw}
 for tag in ('GEOMETRY ','RATING ','OUTCOME '):
  match=re.search(re.escape(tag)+r'(.*)',raw)
  if match:row[tag.split()[0].lower()]=match[1].strip()
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
