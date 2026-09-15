"""Read-only comparison of the tracked source baselines; writes evidence only."""
from pathlib import Path
import difflib, hashlib, json

EV=Path(__file__).resolve().parent
ROOT=EV.parents[3]
B=json.loads((EV.parent/'20260911-step1/baseline.json').read_text(encoding='utf-8'))
EXT={'.c','.h','.s','.sct','.ps1','.py','.uvprojx','.ioc'}
def text(path):
    raw=path.read_bytes()
    for encoding in ('utf-8-sig','gb18030','latin-1'):
        try:return raw.decode(encoding).replace('\r\n','\n'),encoding
        except UnicodeDecodeError:pass
    raise RuntimeError(f'Unknown encoding: {path}')

def files(name):
    data=B['repositories'][name]; repo=ROOT/data['repository_root']
    prefix=data['project_scope'].rstrip('/')+'/' if data['project_scope']!='.' else ''
    result={}
    for f in data['archive']['files']:
        path=repo/f['path']
        assert hashlib.sha256(path.read_bytes()).hexdigest()==f['sha256'],f['path']
        rel=f['path'][len(prefix):]
        if Path(rel).suffix.lower() in EXT and not rel.startswith(('OTA_Artifacts/','App_Delivery/')):
            result[rel]=path
    return result

keep={'BSP/C8721.c','BSP/C8721.h','BSP/screen_c8721.c','BSP/screen_c8721.h',
      'BSP/temp.c','BSP/config.h','COP/main_control.c','Core/Src/main.c',
      'Core/Src/system_stm32f0xx.c','MDK-ARM/tw66gw02_direct.sct','BSP/power_latch.h'}
transfer={'OS/TaskScheduler.c','OS/TaskScheduler.h','Core/Inc/usart.h','Core/Src/usart.c',
          'MDK-ARM/startup_stm32f030x8.s','COP/ota.h','BSP/boot_api.h',
          'Bootloader/boot_security.h'}
reference={'BuildV104Delivery.ps1','MDK-ARM/tw66gw02.sct'}
a=files('main'); b=files('snapshot13'); rows=[]; diffs=[]
for rel in sorted(a.keys()|b.keys()):
    aa,ae=text(a[rel]) if rel in a else ('',None)
    bb,be=text(b[rel]) if rel in b else ('',None)
    status='SAME' if rel in a and rel in b and aa==bb else 'CHANGED' if rel in a and rel in b else 'SOURCE_ONLY' if rel in b else 'MAIN_ONLY'
    if status=='SAME': action='保留 main（相同）'
    elif rel in keep: action='保留 main；不引入显示/关机策略变化'
    elif rel in transfer or rel.startswith('ThirdParty/'): action='迁移；按批次验证'
    elif rel in reference or rel.startswith(('MDK-ARM/RTE/','MDK-ARM/tw66gw02/')) or rel=='MDK-ARM/boot_image.s': action='保留 main 生成/工程入口；参考件不直接复制'
    elif rel.startswith('Tools/PC_BLE_Test/'): action='迁移为可选联调工具；不打包虚拟环境'
    else: action='需适配；见模块清单'
    rows.append({'path':rel,'status':status,'action':action,'main_encoding':ae,'source_encoding':be,
                 'main_sha256':hashlib.sha256(a[rel].read_bytes()).hexdigest() if rel in a else None,
                 'source_sha256':hashlib.sha256(b[rel].read_bytes()).hexdigest() if rel in b else None})
    if status=='CHANGED':
        diffs.extend(difflib.unified_diff(aa.splitlines(keepends=True),bb.splitlines(keepends=True),fromfile='main/'+rel,tofile='snapshot13/'+rel,n=3))
summary={k:sum(row['status']==k for row in rows) for k in ('SAME','CHANGED','SOURCE_ONLY','MAIN_ONLY')}
(EV/'source-comparison.json').write_text(json.dumps({'main_head':B['repositories']['main']['head'],'source_head':B['repositories']['snapshot13']['head'],
 'comparison':'Tracked code/config/scripts; UTF-8 BOM/newline normalized, legacy text decoded GB18030 with byte-preserving Latin-1 fallback; not a semantic-equivalence proof',
 'summary':summary,'files':rows},ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
(EV/'source-differences.txt').write_text(''.join(diffs),encoding='utf-8')
(EV/'file-actions.md').write_text('# 逐文件迁移分类\n\n[主计划](../../../../plan.md) · [模块与批次说明](README.md)\n\n仅比较受跟踪源码、配置和脚本；以下是后续实施分类，并非已迁移结果。\n\n| 文件 | 比较结果 | 处理 |\n| --- | --- | --- |\n'+''.join(f"| `{f['path']}` | {f['status']} | {f['action']} |\n" for f in rows),encoding='utf-8')
print(json.dumps(summary))
