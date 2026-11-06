import json,os,re,subprocess,sys
from pathlib import Path
phase,binary,asan_binary,directory=sys.argv[1:];root=Path(directory)
rows={}
for mode in ('operators','control'):
 env=os.environ|{'T324_MODE':mode}
 result=subprocess.run([str(Path(binary).resolve()),"-v"],env=env,cwd=Path(binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
 raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-{mode}.log').write_text(raw)
 row={'exit':result.returncode,'passed':'native-c/opblock-324 finished: 1/1 passed' in raw}
 for tag in ('GEOMETRY ','RATING ','OUTCOME '):
  match=re.search(re.escape(tag)+r'(.*)',raw)
  if match:row[tag.split()[0].lower()]=match[1].strip()
 rows[mode]=row
 print(mode,row)
# Leak proof: the operator formula under LeakSanitizer (exit code tolerated).
env=os.environ|{'T324_MODE':'operators','ASAN_OPTIONS':'detect_leaks=1'}
result=subprocess.run([str(Path(asan_binary).resolve()),"-v"],env=env,cwd=Path(asan_binary).resolve().parent,timeout=180,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True)
raw=re.sub(r'\x1b\[[0-9;]*m','',result.stdout);(root/f'{phase}-asan-operators.log').write_text(raw)
blocks=[b for b in raw.split('Direct leak of ')[1:]]
rows['asan']={'passed':'native-c/opblock-324 finished: 1/1 passed' in raw,
 'summary':(re.search(r'SUMMARY: AddressSanitizer: (.*)',raw).group(1) if 'SUMMARY: AddressSanitizer' in raw else 'no leaks reported'),
 'operator_block_leaks':sum(1 for b in blocks if 'in adjust_order_ops_depth_block' in b)}
print('asan',rows['asan'])
(root/f'{phase}-results.json').write_text(json.dumps(rows,indent=2)+'\n')
