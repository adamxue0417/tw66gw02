"""Read-back verification and generated step 4 completion report. No firmware mutation."""
from pathlib import Path
from urllib.parse import unquote
import hashlib, json, os, re, shutil, subprocess, zipfile
from step4_prepare import ROOT, OUT, BACK, MAIN, V14, sha, save
from step4_delivery import DELIVERY, check_images

def read(p):
    data=p.read_bytes()
    if data.startswith((b'\xff\xfe',b'\xfe\xff')):return data.decode('utf-16')
    try:return data.decode('utf-8-sig')
    except UnicodeDecodeError:return data.decode('gb18030')

def main():
    protected=json.loads(read(OUT/'清理前源码配置指纹.json'))
    changed=[x['path'] for x in protected if not (ROOT/x['path']).is_file() or sha(ROOT/x['path'])!=x['sha256']]
    assert not changed, changed
    packages=[]
    for package in sorted((DELIVERY/'MCU固件').iterdir()):
        if not package.is_dir():continue
        listed=[]
        for line in read(package/'SHA256SUMS.txt').splitlines():
            expected,rel=line.split('  ',1)
            p=(package/rel).resolve();assert p.is_relative_to(package.resolve())
            assert sha(p)==expected,p
            listed.append(rel)
        assert set(listed)=={p.relative_to(package).as_posix() for p in package.rglob('*') if p.is_file() and p.name!='SHA256SUMS.txt'} | {p.relative_to(package).as_posix() for p in package.rglob('SHA256SUMS.txt') if p.parent!=package}
        meta=json.loads(read(package/'delivery-manifest.json'))
        for row in meta['files']:
            assert sha(ROOT/row['source'])==row['sha256']==sha(package/row['file'])
        assert sha(package/meta['source_fingerprint_file'])==meta['source_fingerprint_sha256']
        for row in json.loads(read(package/'source-fingerprint.json')):assert sha(ROOT/row['path'])==row['sha256']
        manifest=json.loads(read(next((package/'App_Upload').glob('*.manifest.json'))))
        target=f"mathis_app_v{meta['ota_version']}.bin"
        factory_app='mathis_secure_app_v102.bin' if meta['factory_version']==102 else f"mathis_app_v{meta['factory_version']}.bin"
        checks=check_images(package,{'ota_app':target,'factory_app':factory_app,'boot_bin':next((package/'Bootloader').glob('*.bin')).name,'factory_bin':next((package/'Factory_Flash').glob('*.bin')).name,'slot_kib':meta['slot_kib']})
        for p in package.rglob('*.zip'):
            with zipfile.ZipFile(p) as z:
                assert z.testzip() is None,p
                assert not any(n.lower().endswith(('.pem','.key','.pfx','.p12')) for n in z.namelist()),'Key material must not be distributed'
        packages.append({'id':package.name,'checksummed_files':len(listed),'checks':checks})
    moves=json.loads(read(OUT/'安装包与模块迁移计划.json'))
    for row in moves:
        assert not (ROOT/row['old_path']).exists()
        assert sha(ROOT/row['new_path'])==row['sha256']
    assert all(not (ROOT/name).exists() for name in ['apk','ble','emb1061'])
    backup=json.loads(read(OUT/'中间文件备份.json'))
    assert sha(ROOT/backup['archive'])==backup['archive_sha256']
    restored=[]
    with zipfile.ZipFile(ROOT/backup['archive']) as z:
        for project in [MAIN,V14]:
            prefix=project.relative_to(ROOT).as_posix()+'/'
            row=next(x for x in backup['files'] if x['path'].startswith(prefix))
            target=BACK/'restore-check'/row['path'];target.parent.mkdir(parents=True,exist_ok=True)
            with z.open(row['path']) as src,target.open('wb') as dst:shutil.copyfileobj(src,dst)
            assert sha(target)==row['sha256'];restored.append(row['path'])
    current=[]
    for project in [MAIN,V14]:
        bases=[project/'Bootloader/build',project/'Bootloader/build_min']
        bases += [p for p in (project/'MDK-ARM').iterdir() if p.is_dir() and (p.name.startswith('tw66gw02') or p.name in {'runs','.build-runs'})]
        for base in bases:
            if base.exists():current.extend(p for p in base.rglob('*') if p.is_file() and p.suffix.lower() in {'.o','.crf','.d'})
    after=sum(p.stat().st_size for p in current)
    stats={'before_files':backup['file_count'],'before_bytes':backup['original_bytes'],'after_rebuild_files':len(current),'after_rebuild_bytes':after,'reduction_in_selected_intermediates_bytes':backup['original_bytes']-after,'backup_zip_bytes':backup['archive_bytes'],'reduction_less_backup_bytes':backup['original_bytes']-after-backup['archive_bytes'],'scope':'Only .o/.crf/.d in the explicitly selected compiler output directories; not total disk free-space measurement','restored_samples':restored}
    save(OUT/'清理与空间统计.json',stats)
    builds={}
    for name in ['main_ota_test','snapshot14_ota_test']:
        assert '162 interrupted swap/rollback cases passed' in read(OUT/(name+'.log'))
        builds[name]='PASS / 162 cases'
    for name,project in [('main',MAIN),('snapshot14',V14)]:
        for version in [100,101]:
            log=project/f'OTA_Artifacts/build_v{version}.log'
            assert re.search(r'0 Error\(s\), 0 Warning\(s\)',read(log)),log
            shutil.copy2(log,OUT/f'{name}_app_v{version}.log')
    log=MAIN/'OTA_Artifacts/build_factory_keil.log'
    assert re.search(r'0 Error\(s\), 0 Warning\(s\)',read(log))
    shutil.copy2(log,OUT/'main_factory_keil.log')
    gitchecks=[]
    for rel,wanted in [('05_发布与交付/README.md',False),('05_发布与交付/MCU固件/main_v100-to-v101_DEV/delivery-manifest.json',False),('05_发布与交付/MCU固件/main_v100-to-v101_DEV/Factory_Flash/mathis_factory_v100.bin',True),('05_发布与交付/手机APP/Android/v3.6.9_build368/application-aaa6c0d1-46c7-4075-8fe6-ca621db3e285.apk',True),('01_固件工程/main/MDK-ARM/startup_stm32f030x8.s',False),('90_历史原始包/整理前备份/20260910-step4/compiler-intermediates.zip',True)]:
        proc=subprocess.run(['git','-C',str(ROOT),'check-ignore','--no-index','-q',rel]);assert proc.returncode in (0,1)
        actual=proc.returncode==0;assert actual==wanted,rel
        gitchecks.append({'path':rel,'ignored':actual,'pass':True})
    result={'date':'2026-09-10','protected_source_config_files':len(protected),'unexpected_source_changes':changed,'mobile_ble_files_verified':len(moves),'packages':packages,'build_tests':builds,'main_factory':'PASS / 0 errors, 0 warnings','ignore_checks':gitchecks,'space':stats,'limitations':['No hardware/BLE/app installation testing','Existing DEV signatures preserved, not cryptographically reverified in this step','Original Bootloader build commits unknown for historical secure deliveries','Step 3 pre-existing v12 factory/workbook issues remain']}
    save(OUT/'最终验证.json',result)
    mib=lambda n:f'{n/1048576:.1f}'
    report=f'''# 第四步：交付整理与清理报告

[返回项目入口](../README.md) · [交付选择表](../05_发布与交付/README.md)

2026-09-10，第四步目录整理完成。四组 MCU DEV 交付快照、手机 APP 和 BLE 模块固件已归集；指定编译中间文件完成备份、清理和重建验证。

## 交付目录

MCU 按 main v100→v101、snapshot14 v100→v101、snapshot12 v102→v103、snapshot13 v103→v104（Bootloader v2）分别保存。每组包含 Factory_Flash、App_Upload、Debug_Only、Bootloader、Verification、源码指纹、交付 manifest 和完整 SHA-256 清单。原工程输出保留；共核对 {sum(x['checksummed_files'] for x in packages)} 个清单内文件。

原 apk 的 Android 3.6.9/build368、iOS 3.6.9/build98 迁入手机APP；原 ble/emb1061 的四个固件或配置文件迁入 BLE模块固件。六个文件均保持原字节和原文件名，根目录三个空目录已移除。详见[路径映射](证据/第四步/安装包与模块迁移计划.json)与[安装包登记](../05_发布与交付/现有安装包与模块固件.json)。

交付件没有烧录或上传。main/14 是零签名占位的开发 OTA；12/13 是既有 DEV 签名件。v103/v104 应用与第三步当前源码重编译结果相同；其历史 Bootloader/Factory 原构建提交未记录，当前源码指纹不冒充历史构建证明。BLE 协议、板卡修订及 MCU/BLE/手机 APP 适配组合待核实。

## 清理范围与空间

仅选择主线和 14 的指定 MDK-ARM 编译输出目录、Bootloader/build 与 build_min 中的 .o/.crf/.d。Git 跟踪检查通过，{backup['file_count']:,} 个文件全部先压缩备份、逐成员解压核对，再逐文件删除。重新构建会生成需要的中间文件，因此目录中仍可见这类文件。

| 项目 | 体积 |
| --- | --- |
| 清理前选定中间文件 | {mib(backup['original_bytes'])} MiB |
| 重建后同范围中间文件 | {mib(after)} MiB |
| 同范围体积减少 | {mib(backup['original_bytes']-after)} MiB |
| 保留的压缩恢复包 | {mib(backup['archive_bytes'])} MiB |
| 扣除恢复包后的差额 | {mib(stats['reduction_less_backup_bytes'])} MiB |

这些数字只统计选定中间文件及恢复 ZIP，不是整盘可用空间测量；新交付说明和复制的 MCU 产物也占用空间。[机器可读统计](证据/第四步/清理与空间统计.json)保留精确字节数。

保留源码、启动 .s、链接 .sct、boot_image.o、已有固件/manifest/日志、历史快照、.git/.git.disabled、私钥、依赖环境及 14 的重构基线。没有把历史输出或虚拟环境当成已确认可重建的缓存删除。两份根目录原始 PDF/PPT 与历史资源目录继续保留，公共维护副本仍从索引进入。

## 验证与恢复

- 主线/14 清理后 OTA 应用构建：各 v100/v101 均 0 错误、0 警告；主线工厂构建通过。
- 14 启动诊断重新生成；两条线 OTA 文件/分区检查及各 162 个中断恢复用例通过。
- 四包 OTA 长度/SHA-256/CRC、应用内容与 Factory/Bootloader 分区对应检查通过。
- {len(protected)} 个维护源码/配置指纹无变化；六个手机/BLE 文件迁移校验通过。
- 恢复包全量解压校验 {backup['verified_decompressed_files']:,} 文件通过，另实际恢复主线/14 各一个样本并核对通过。

[最终验证记录](证据/第四步/最终验证.json) · [链接检查](证据/第四步/最终链接检查.json) · [备份与恢复说明](../90_历史原始包/整理前备份/20260910-step4/README.md)。原有第三步证据保留，不重写为本次结果。

根忽略规则补充交付二进制扩展名，README、manifest、校验清单仍可纳入版本管理。本轮没有 Git 暂存、提交或推送；12/13 仓库状态保持第三步结束时的状态。

## 后续维护

目录架构和索引整理已完成，后续按[交付检查表](../05_发布与交付/交付检查表.md)维护新版本。12 的旧工厂入口分区不兼容、14 工作簿测试的原有失败仍见[第三步报告](第三步迁移与验证报告.md)；本步未修复或改记为通过。实机烧录、真实 BLE、手机安装与量产验证尚未执行。
'''
    (ROOT/'00_项目索引/第四步交付整理与清理报告.md').write_text(report,encoding='utf-8')
    # Maintained navigation, public documents and release docs; no blanket claims for historical internals.
    docs=list(ROOT.glob('*.md'))
    for folder in ['00_项目索引','02_需求与协议','03_硬件资料','04_测试与联调','05_发布与交付']:
        docs += [p for p in (ROOT/folder).rglob('*.md') if '实验工程' not in p.parts and '证据' not in p.parts]
    docs += [ROOT/'01_固件工程/README.md',ROOT/'06_工具与参考/README.md',ROOT/'90_历史原始包/README.md',BACK/'README.md']
    save(OUT/'最终链接检查.json',{'status':'checking'})
    missing=[];count=0
    for p in docs:
        text=re.sub(r'```.*?```','',read(p),flags=re.S)
        for m in re.finditer(r'!?\[[^\]]*\]\((<[^>]+>|[^\s)]+)\)',text):
            url=m.group(1).strip('<>')
            if url.startswith(('http:','https:','mailto:','#')):continue
            target=unquote(url.split('#',1)[0]);count+=1
            if not (p.parent/target).exists():missing.append({'file':p.relative_to(ROOT).as_posix(),'link':url})
    links={'markdown_files':len(docs),'local_links_checked':count,'missing':missing,'scope':'Maintained root/index/public/release navigation; excludes historical engineering internals and evidence documents'}
    save(OUT/'最终链接检查.json',links)
    assert not missing,missing
    print(json.dumps({'protected_sources':len(protected),'packages':len(packages),'mobile_ble':len(moves),'links_checked':count,'space':stats},ensure_ascii=False))

if __name__=='__main__':main()
