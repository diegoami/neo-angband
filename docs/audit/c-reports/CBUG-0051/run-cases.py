import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
cases=('two-phase','no-morgoth-phase','single')
rows={}
for mode in cases:
 env=os.environ|{'CBUG0051_MODE':mode,'CBUG0051_PROFILE':str(root/'profile')}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/CBUG-0051 finished: 1/1 passed' in raw}
 for tag,key in (('KILL index','kills'),('MORGOTH','morgoth'),('LAUNCH','launch'),('OUTCOME','outcome'),('TEMP phase1','temp1'),('TEMP phase2','temp2')):
  match=re.search(re.escape(tag)+r'(.*)',raw)
  if match:row[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match[1])}
 if 'kills' not in row:
  row['kills']=[{k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',l)} for l in raw.splitlines() if l.startswith('KILL ')]
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
