import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,asan_binary,directory=sys.argv[1:];root=Path(directory)
rows={}
for mode in ('glow','control'):
 env=os.environ|{'T311_MODE':mode,'T311_PROFILE':str(root/'profile')}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/glowoverflow-311 finished: 1/1 passed' in raw}
 for tag in ('GEOMETRY ','RATING ','OUTCOME '):
  match=re.search(re.escape(tag)+r'(.*)',raw)
  if match:row[tag.split()[0].lower()]=match[1].strip()
 rows[mode]=row
 print(mode,row)
# Memory-safety proof: the overflowing scan under AddressSanitizer.
# intercept_strcmp=0: the pristine base trips an unrelated strcmp
# interceptor read in borg_init's monster-name sort.
env=os.environ|{'T311_MODE':'glow','T311_PROFILE':str(root/'profile'),
 'ASAN_OPTIONS':'detect_leaks=0:intercept_strcmp=0'}
result=subprocess.run([str(Path(asan_binary).resolve()),"-v"],env=env,cwd=Path(asan_binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-asan-glow.log').write_text(raw)
summary=re.search(r'SUMMARY: AddressSanitizer: (\S+) [^ ]*:(\d+) in (\S+)',raw)
write=re.search(r'(WRITE|READ) of size (\d+)',raw)
rows['asan']={'passed':'native-c/glowoverflow-311 finished: 1/1 passed' in raw,
 'summary':summary.group(0) if summary else 'no sanitizer report',
 'write':(write.group(0) if write else None),
 'in_flow_light':'in borg_flow_light' in raw}
print('asan',rows['asan'])
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
