"""Inventory and map relocation; does not move or edit existing projects."""
from pathlib import Path
import csv, hashlib, json, os

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / '00_项目索引/证据/第三步'
MAIN = '01_固件工程/main'
names = ['Bootloader','BSP','COP','Core','Diagnostics','Drivers','MDK-ARM','OS','tests','tools',
         '.mxproject','tw66gw02.ioc','CHANGELOG.md','EMB1082_蓝牙接口说明.md','OTA_DEVELOPMENT_README.md','OTA_烧录与APP联调说明.md']
names += [p.name for p in ROOT.glob('*.ps1')]
mapping = [{'source': n, 'target': MAIN + '/' + n, 'group': 'main'} for n in names]
labels = ['early-baseline','incomplete','temperature-errors','calibration','units-display','early-ota','direct-boot','mathis-ble','secure-ota-experiment','development-ota','powerhold-incomplete']
mapping += [{'source':str(n),'target':f'01_固件工程/history/snapshot-{n:02d}_{label}','group':'history'} for n,label in enumerate(labels,1)]
mapping += [{'source':str(n),'target':'01_固件工程/development/'+label,'group':'development'} for n,label in [(12,'snapshot-12_secure-ota'),(13,'snapshot-13_poweroff-v104'),(14,'snapshot-14_refactor-from-10')]]
mapping += [{'source':n,'target':'04_测试与联调/实验工程/'+n,'group':'experiments'} for n in ['TEST','ostest','eide']]
mapping += [{'source':'JLink','target':'06_工具与参考/JLink','group':'tools'}, {'source':'tw66gw02.jflash','target':'06_工具与参考/JFlash配置/tw66gw02_snapshot10.jflash','group':'tools'}]
OUT.mkdir(exist_ok=False)
excluded = {'private_keys','deps','.venv','__pycache__'}
rows=[]; excluded_paths=[]; dirs=[]
def digest(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def fail(err):raise err
for m in mapping:
    src=ROOT/m['source'];dest=ROOT/m['target']
    assert src.exists() and not dest.exists(),m
    assert src.resolve().is_relative_to(ROOT) and dest.resolve().is_relative_to(ROOT)
    paths=[]
    if src.is_file():paths=[src]
    else:
        for d,ds,ns in os.walk(src,onerror=fail):
            for n in list(ds):
                if n in excluded:
                    excluded_paths.append((Path(d)/n).relative_to(ROOT).as_posix());ds.remove(n)
            dirs.append(Path(d).relative_to(ROOT).as_posix())
            paths += [Path(d)/n for n in ns]
    for p in sorted(paths):
        rel=p.relative_to(ROOT).as_posix()
        suffix=rel[len(m['source']):]
        rows.append({'old_path':rel,'new_path':m['target']+suffix,'group':m['group'],'bytes':p.stat().st_size,'sha256':digest(p)})
(OUT/'路径映射.json').write_text(json.dumps(mapping,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
with (OUT/'迁移前文件指纹.csv').open('w',encoding='utf-8-sig',newline='') as f:
    w=csv.DictWriter(f,fieldnames=list(rows[0]));w.writeheader();w.writerows(rows)
(OUT/'盘点范围.json').write_text(json.dumps({'files':len(rows),'source_bytes':sum(x['bytes'] for x in rows),'excluded_from_content_hash_only':excluded_paths,'directories':dirs,'note':'Excluded dependency/private directories travel with their parent; their contents were not read or hashed.'},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'moves':len(mapping),'files':len(rows),'excluded_content_hash':excluded_paths},ensure_ascii=False))
