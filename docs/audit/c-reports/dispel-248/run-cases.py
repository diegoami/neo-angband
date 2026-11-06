import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
cases=('living','undead','bolt')
rows={}
for mode in cases:
 env=os.environ|{'D248_MODE':mode,'D248_PROFILE':str(root/'profile')}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/dispel-248 finished: 1/1 passed' in raw}
 for tag,key in (('GATE ','gate'),('LAUNCH ','launch'),('RATING ','rating'),('OUTCOME ','outcome')):
  match=re.search(re.escape(tag)+r'(.*)',raw)
  if match:row[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match[1])}
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
