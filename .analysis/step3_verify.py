"""Verify relocation against the pre-move byte manifest, before edits/builds."""
from pathlib import Path
import argparse,csv,hashlib,json
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'00_项目索引/证据/第三步'
parser=argparse.ArgumentParser();parser.add_argument('group');args=parser.parse_args()
rows=[r for r in csv.DictReader((OUT/'迁移前文件指纹.csv').open(encoding='utf-8-sig',newline='')) if r['group']==args.group]
bad=[]
for row in rows:
    p=ROOT/row['new_path']
    if not p.is_file():bad.append([row['new_path'],'missing']);continue
    with p.open('rb') as f:sha=hashlib.file_digest(f,'sha256').hexdigest()
    if sha!=row['sha256']:bad.append([row['new_path'],'changed'])
result={'group':args.group,'checked':len(rows),'mismatches':bad,'phase':'After directory moves, before intentional link/config edits and builds'}
(OUT/(args.group+'-move-verification.json')).write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'group':args.group,'checked':len(rows),'mismatch_count':len(bad),'sample':bad[:5]},ensure_ascii=False));raise SystemExit(bool(bad))
