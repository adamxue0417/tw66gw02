"""Record final source integrity and Git boundary checks after build verification."""
from pathlib import Path
import csv,hashlib,json,subprocess,shutil
ROOT=Path(__file__).resolve().parents[1];OUT=ROOT/'00_项目索引/证据/第三步'
rows=list(csv.DictReader((OUT/'迁移前文件指纹.csv').open(encoding='utf-8-sig',newline='')))
extensions={'.c','.h','.s','.sct','.ld','.uvprojx','.ioc','.ps1','.py','.jflash','.yml','.code-workspace'}
expected={'01_固件工程/main/PrepareFactoryBootImage.ps1','01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02/BuildV104Delivery.ps1','06_工具与参考/JFlash配置/tw66gw02_snapshot10.jflash'}
checked=0;changed=[];missing=[]
for x in rows:
    if Path(x['new_path']).suffix.lower() not in extensions:continue
    # Script tests generate .ps1/.c/.uvprojx probes; these are not maintained sources.
    if '/tests/build/' in x['new_path'] or '/MDK-ARM/runs/' in x['new_path']:continue
    p=ROOT/x['new_path'];checked+=1
    if not p.exists():missing.append(x['new_path']);continue
    with p.open('rb') as f:h=hashlib.file_digest(f,'sha256').hexdigest()
    if h!=x['sha256']:changed.append({'path':x['new_path'],'before_sha256':x['sha256'],'after_sha256':h,'expected':x['new_path'] in expected})
def git(path,*args):
    p=subprocess.run(['git','-C',str(ROOT/path),*args],capture_output=True,text=True,encoding='utf-8')
    return {'exit_code':p.returncode,'output':p.stdout.strip(),'error':p.stderr.strip()}
repos={}
for name,path in [('root','.'),('12','01_固件工程/development/snapshot-12_secure-ota/tw66gw02'),('13','01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02'),('14','01_固件工程/development/snapshot-14_refactor-from-10')]:
    repos[name]={'path':path,'root':git(path,'rev-parse','--show-toplevel'),'head':git(path,'rev-parse','--verify','HEAD'),'branch':git(path,'symbolic-ref','--short','HEAD'),'staged':git(path,'diff','--cached','--name-only')}
ignores=[]
for repo,path,wanted in [('.', '01_固件工程/main/BSP/config.h',False),('.', '01_固件工程/main/MDK-ARM/startup_stm32f030x8.s',False),('.', '01_固件工程/main/MDK-ARM/tw66gw02_factory.sct',False),('.', '01_固件工程/main/OTA_Artifacts/mathis_ota_v101.ota',True),('.', '01_固件工程/development/snapshot-14_refactor-from-10/tw66gw02/BSP/config.h',True),('.', '01_固件工程/history/snapshot-10_development-ota/tw66gw02/BSP/config.h',True),('01_固件工程/development/snapshot-14_refactor-from-10','tw66gw02/BSP/config.h',False),('01_固件工程/development/snapshot-14_refactor-from-10','outputs/optimization/baseline/build_mathis_test_workbook.ps1',False),('01_固件工程/development/snapshot-14_refactor-from-10','outputs/optimization/deps/unicorn/__init__.py',True)]:
    result=git(repo,'check-ignore','--no-index','-q',path);actual=result['exit_code']==0
    ignores.append({'repository':repo,'path':path,'expected_ignored':wanted,'actual_ignored':actual,'pass':actual==wanted and result['exit_code'] in (0,1)})
result={'checked_source_config_files':checked,'changed':changed,'missing':missing,'unexpected_changes':[x for x in changed if not x['expected']],'repositories':repos,'ignore_checks':ignores,'note':'Generated build outputs are excluded from this final source check; pre-edit full move checks cover 15919 files.'}
(OUT/'最终源码与仓库检查.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
# Keep small build logs with the index, leaving bulk validation artifacts in the backup area.
logs={'main_factory.log':'01_固件工程/main/OTA_Artifacts/build_factory_keil.log','main_app_v100.log':'01_固件工程/main/OTA_Artifacts/build_v100.log','main_app_v101.log':'01_固件工程/main/OTA_Artifacts/build_v101.log','v14_app_v100.log':'01_固件工程/development/snapshot-14_refactor-from-10/tw66gw02/OTA_Artifacts/build_v100.log','v14_app_v101.log':'01_固件工程/development/snapshot-14_refactor-from-10/tw66gw02/OTA_Artifacts/build_v101.log','v14_c_regressions.log':'01_固件工程/development/snapshot-14_refactor-from-10/tw66gw02/tests/build/results.log','v103_app.log':'90_历史原始包/整理前备份/20260909-step3/validation/v103/build_step3.log','v104_app.log':'90_历史原始包/整理前备份/20260909-step3/validation/v104/build_step3.log'}
for name,path in logs.items():shutil.copy2(ROOT/path,OUT/name)
print(json.dumps({'checked':checked,'changed':changed,'missing':missing,'ignore_checks_passed':sum(x['pass'] for x in ignores),'ignore_checks_total':len(ignores)},ensure_ascii=False))
raise SystemExit(bool(missing or result['unexpected_changes'] or not all(x['pass'] for x in ignores)))
