import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,directory=sys.argv[1:];root=Path(directory)
cases=('no-target','live-shot','simulated')
rows={}
for mode in cases:
 env=os.environ|{'M254_MODE':mode,'M254_PROFILE':str(root/'profile'),'ASAN_OPTIONS':'detect_leaks=0:intercept_strcmp=0'}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/missile-254 finished: 1/1 passed' in raw,
      'asan':'AddressSanitizer' in raw,
      'oob_read':('heap-buffer-overflow' in raw and 'READ of size' in raw and 'borg-fight-attack.c' in raw)}
 for tag,key in (('DECISION','decision'),('OUTCOME','outcome')):
  match=re.search(re.escape(tag)+r' (.*)',raw)
  if match:row[key]={k:int(v) for k,v in re.findall(r'(\w+)=(-?\d+)',match[1])}
 rows[mode]=row
 print(mode,row)
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
