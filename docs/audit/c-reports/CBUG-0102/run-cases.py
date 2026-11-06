import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
cases=('stairs','clear-floor','pairing')
rows={}
for mode in cases:
 env=os.environ|{'CBUG0102_MODE':mode,'CBUG0102_PROFILE':str(root/'profile'),'ASAN_OPTIONS':'detect_leaks=0:intercept_strcmp=0'}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/CBUG-0102 finished: 1/1 passed' in raw,
      'asan':'AddressSanitizer' in raw,
      'underflow':('heap-buffer-overflow' in raw and 'WRITE of size' in raw and 'borg-fight-defend.c' in raw),
      'defend_site':'borg-fight-defend.c' in raw}
 for tag,key in (('TRACK phaseA','phaseA'),('TRACK phaseB_acted','phaseB'),('MESSAGE','message'),('OUTCOME','outcome'),('DECISION phaseA','decisionA'),('DECISION phaseB','decisionB')):
  match=re.search(re.escape(tag)+r' (.*)',raw)
  if match:row[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match[1])}
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
