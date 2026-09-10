"""Build traceable delivery snapshots from explicit existing artifacts; no signing/flashing."""
from pathlib import Path
import hashlib, json, os, shutil, subprocess, zlib
from step4_prepare import ROOT, OUT, MAIN, V14, sha, save

DELIVERY = ROOT/'05_发布与交付'
V12 = ROOT/'01_固件工程/development/snapshot-12_secure-ota/tw66gw02'
V13 = ROOT/'01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02'

def source_inventory(project):
    rows=[]
    for parent, dirs, files in os.walk(project):
        dirs[:] = [d for d in dirs if d not in {'.git','.git.disabled','.venv','venv','__pycache__','build','build_min','runs','.build-runs','OTA_Artifacts','App_Delivery','private_keys'} and not d.startswith('tw66gw02_ota_')]
        for name in files:
            p=Path(parent)/name
            if p.suffix.lower() in {'.c','.h','.s','.sct','.ld','.uvprojx','.ioc','.ps1','.py'} and '.generated.' not in name and name!='boot_image.s':
                rows.append({'path':p.relative_to(ROOT).as_posix(),'sha256':sha(p)})
    rows.sort(key=lambda x:x['path'])
    return rows

def check_images(package, meta):
    ota=next((package/'App_Upload').glob('*.ota'))
    manifest=json.loads(next((package/'App_Upload').glob('*.manifest.json')).read_text(encoding='utf-8-sig'))
    blob=ota.read_bytes();n=manifest['application_size'];app=blob[:n]
    assert len(blob)==manifest['artifact_size']==n+manifest['signature_size']
    assert sha(ota)==manifest.get('artifact_sha256',manifest.get('sha256')).lower()
    assert f'0x{zlib.crc32(blob):08X}'==manifest['crc32_iso_hdlc'].upper().replace('0X','0x')
    debug=package/'Debug_Only'/meta['ota_app'];assert debug.read_bytes()==app
    if manifest.get('application_sha256'):assert sha(debug)==manifest['application_sha256'].lower()
    if manifest['development_signature_placeholder']:assert blob[n:]==bytes(384)
    else: assert len(blob[n:])==384 and any(blob[n:])
    boot=(package/'Bootloader'/meta['boot_bin']).read_bytes()
    factory=(package/'Factory_Flash'/meta['factory_bin']).read_bytes()
    factory_app=(package/'Debug_Only'/meta['factory_app']).read_bytes()
    offset=int(manifest['application_base'],16)-0x08000000
    assert len(boot)<=offset and factory[:len(boot)]==boot
    assert factory[len(boot):offset]==b'\xff'*(offset-len(boot))
    assert factory[offset:]==factory_app
    assert len(blob)<=meta['slot_kib']*1024
    return {'date':'2026-09-10','ota_size_sha256_crc32':'PASS','ota_payload_matches_application':'PASS','factory_boot_partition_and_application':'PASS','signature':'384-byte zero DEV placeholder checked' if manifest['development_signature_placeholder'] else 'Existing DEV signature preserved; cryptographic verification not repeated in step 4','hardware':'PENDING','application_base':manifest['application_base']}

def package_one(label,project,factory_version,ota_version,bootver=None):
    package=DELIVERY/'MCU固件'/label
    assert not package.exists(),f'Refuse overwrite: {package}'
    package.mkdir(parents=True)
    mapping=[]
    def cp(src,rel):
        assert src.is_file(),src
        dst=package/rel;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(src,dst)
        assert sha(src)==sha(dst)
        mapping.append({'source':src.relative_to(ROOT).as_posix(),'file':dst.relative_to(package).as_posix(),'bytes':dst.stat().st_size,'sha256':sha(dst)})
    artifacts=project/'OTA_Artifacts'
    if bootver is None:
        release=artifacts/'Release_v100_to_v101'
        for src in sorted(release.rglob('*')):
            if src.is_file():cp(src,'Verification/Original_release_SHA256SUMS.txt' if src.name=='SHA256SUMS.txt' else src.relative_to(release))
        cp(project/'Bootloader/build/mathis_bootloader.bin','Bootloader/mathis_bootloader.bin')
        meta={'factory_bin':'mathis_factory_v100.bin','factory_app':'mathis_app_v100.bin','ota_app':'mathis_app_v101.bin','boot_bin':'mathis_bootloader.bin','slot_kib':27}
        for name in ['build_v100.log','build_v101.log']:
            cp(artifacts/name,'Verification/'+name)
        test_log='main_ota_test.log' if project==MAIN else 'snapshot14_ota_test.log'
        cp(OUT/test_log,'Verification/'+test_log)
        association='本次清理后重新构建并通过 OTA 检查；当前源码指纹对应本次构建。'
    else:
        boot=f'mathis_secure_bootloader_dev_v{bootver}'
        factory='mathis_secure_factory_v102_dev' if bootver==1 else 'mathis_secure_factory_v103_bootv2_dev'
        factory_app='mathis_secure_app_v102' if bootver==1 else 'mathis_app_v103'
        target=f'mathis_app_v{ota_version}'
        for folder,stem in [('Bootloader',boot),('Factory_Flash',factory),('Debug_Only',factory_app),('Debug_Only',target)]:
            for ext in ['.bin','.hex']:cp(artifacts/(stem+ext),folder+'/'+stem+ext)
        for ext in ['.ota','.manifest.json']:
            name=f'mathis_ota_v{ota_version}_dev_signed'+ext
            cp(artifacts/name,'App_Upload/'+name)
            assert sha(artifacts/name)==sha(project/'App_Delivery'/name)
        for name in [f'APP_V{ota_version}_OTA_INTEGRATION.md','SHA256SUMS.txt' if ota_version==103 else 'SHA256SUMS_V104.txt']:
            cp(project/'App_Delivery'/name,'App_Upload/'+name)
        originals=['mathis_v103_dev_ota_package.zip'] if ota_version==103 else ['mathis_pc_ble_v104_test_package.zip']
        for name in originals:cp(project/'App_Delivery'/name,'Original_Packages/'+name)
        for name in [f'V{ota_version}_VALIDATION.log',f'V{ota_version}_SHA256SUMS.txt']:
            cp(artifacts/name,'Verification/Historical/'+name)
        cp(ROOT/f'00_项目索引/证据/第三步/v{ota_version}_app.log',f'Verification/step3_v{ota_version}_app.log')
        rebuilt=ROOT/f'90_历史原始包/整理前备份/20260909-step3/validation/v{ota_version}/step3_app_v{ota_version}.bin'
        equal=sha(rebuilt)==sha(artifacts/(target+'.bin'))
        meta={'factory_bin':factory+'.bin','factory_app':factory_app+'.bin','ota_app':target+'.bin','boot_bin':boot+'.bin','slot_kib':25}
        association=f'既有 DEV 签名交付件；第三步当前源码应用重编译与本包应用字节一致：{equal}。Bootloader/Factory 原始构建提交未记录，当前源码指纹仅供追溯，不能证明其历史构建来源。'
        assert equal,'Historical application no longer matches step 3 rebuilt source'
    rows=source_inventory(project)
    save(package/'source-fingerprint.json',rows)
    head=subprocess.run(['git','-C',str(project),'rev-parse','--verify','HEAD'],capture_output=True,text=True)
    manifest=json.loads(next((package/'App_Upload').glob('*.manifest.json')).read_text(encoding='utf-8-sig'))
    checks=check_images(package,meta)
    metadata={'id':label,'snapshot_date':'2026-09-10','classification':'DEV / 非量产交付','source_project':project.relative_to(ROOT).as_posix(),'git_base_head':head.stdout.strip() if head.returncode==0 else '无提交','git_head_is_exact_build_commit':False,'source_fingerprint_file':'source-fingerprint.json','source_fingerprint_sha256':sha(package/'source-fingerprint.json'),'source_association':association,'factory_version':factory_version,'ota_version':ota_version,'protocol_version':'待核实；公共 BLE 文档 v0.4 Draft 不等同于本包已验证协议版本','application_base':manifest['application_base'],'bootloader_partition_kib':6 if bootver is None else 8,'bootloader_iteration':bootver if bootver else 'development / RSA verification disabled','boot_api_version':manifest.get('boot_api_version','未在 manifest 声明'),'slot_kib':meta['slot_kib'],'hardware':'工程目标 STM32F030C8；板卡修订号及 MCU/BLE/手机 APP 适配组合待实机核实','build_tools':'Keil uVision / ARM Compiler 5.06 update 7；历史 Bootloader 原构建环境仅有现存脚本/日志依据','validation':checks,'files':mapping}
    save(package/'delivery-manifest.json',metadata)
    save(package/'Verification/step4-validation.json',checks)
    (package/'README.md').write_text(f'''# {label}

[返回交付入口](../../README.md)

本目录是本地开发交付快照，保留 DEV 标记，未进行硬件烧录、真实 BLE 联调或量产签核。

| 项目 | 说明 |
| --- | --- |
| 源工程 | `{metadata['source_project']}` |
| 版本 | 工厂 v{factory_version} → OTA v{ota_version} |
| 分区 | Bootloader {metadata['bootloader_partition_kib']} KiB；App `{metadata['application_base']}`；槽 {meta['slot_kib']} KiB |
| Bootloader | {metadata['bootloader_iteration']}；manifest boot_api_version = {metadata['boot_api_version']}，API 号与 Bootloader 迭代号分别记录 |
| 硬件/协议适配 | {metadata['hardware']}；{metadata['protocol_version']} |
| 编译工具 | {metadata['build_tools']} |
| 本次校验 | OTA 大小、SHA-256、CRC、应用内容、工厂镜像和分区一致性通过 |

{association}

- [Factory_Flash](Factory_Flash/)：本包对应 Bootloader 的完整工厂镜像。
- [App_Upload](App_Upload/)：OTA 原始包及相邻 manifest；不可截掉签名尾部。
- [Debug_Only](Debug_Only/)：应用裸镜像，仅供调试，不用于工厂烧录或手机上传。
- [Bootloader](Bootloader/)：与本包工厂镜像匹配的引导程序参考副本。
- [验证记录](Verification/)：本次文件校验和带来源的构建记录；Historical 下是原有日志，不表示本次重新执行签名验证。
- [交付 manifest](delivery-manifest.json)、[源码指纹](source-fingerprint.json)、[全包 SHA-256](SHA256SUMS.txt)。

不同源工程即使版本号相同也不能互换；6 KiB 与 8 KiB 引导分区不能混用。后续正式交付按[交付检查表](../../交付检查表.md)补齐实机验证和适配记录。
''',encoding='utf-8')
    allfiles=sorted(p for p in package.rglob('*') if p.is_file() and p!=package/'SHA256SUMS.txt')
    (package/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.relative_to(package).as_posix()}\n' for p in allfiles),encoding='utf-8')
    return {'id':label,'path':package.relative_to(ROOT).as_posix(),'factory_version':factory_version,'ota_version':ota_version,'application_base':manifest['application_base'],'files':len(allfiles)+1,'validation':checks}

def plan_mobile_moves():
    items=json.loads((DELIVERY/'现有安装包与模块固件.json').read_text(encoding='utf-8'))
    moves=[]
    for item in items:
        old=item['path'];src=ROOT/old;assert sha(src)==item['sha256']
        if old.startswith('apk/'):
            rel=f"手机APP/{item['platform']}/v{item['version']}_build{item['build']}/{item['original_name']}"
        elif old.startswith('ble/'):rel='BLE模块固件/oven_ble_版本待核实/'+item['original_name']
        else:rel='BLE模块固件/EMB1061_版本待核实/'+item['original_name']
        dest='05_发布与交付/'+rel
        assert not (ROOT/dest).exists()
        moves.append({'old_path':old,'new_path':dest,'sha256':item['sha256'],'bytes':item['bytes']})
        item['original_path']=old;item['path']=dest;item['validation_date']='2026-09-10'
    save(OUT/'安装包与模块迁移计划.json',moves)
    save(OUT/'更新后安装包登记.json',items)
    print('Prepared six verified file moves.')

if __name__=='__main__':
    import sys
    if sys.argv[1]=='plan':plan_mobile_moves()
    elif sys.argv[1]=='preserve-original-checksums':
        catalog=json.loads((DELIVERY/'MCU交付目录.json').read_text(encoding='utf-8'))
        for entry in catalog:
            package=ROOT/entry['path']
            meta=json.loads((package/'delivery-manifest.json').read_text(encoding='utf-8'))
            for row in meta['files']:
                if row['file']=='SHA256SUMS.txt':
                    row['file']='Verification/Original_release_SHA256SUMS.txt'
                    shutil.copy2(ROOT/row['source'],package/row['file'])
            save(package/'delivery-manifest.json',meta)
            files=sorted(p for p in package.rglob('*') if p.is_file() and p!=package/'SHA256SUMS.txt')
            (package/'SHA256SUMS.txt').write_text(''.join(f'{sha(p)}  {p.relative_to(package).as_posix()}\n' for p in files),encoding='utf-8')
            entry['files']=len(files)+1
        save(DELIVERY/'MCU交付目录.json',catalog)
    elif sys.argv[1]=='packages':
        catalog=[]
        for args in [('main_v100-to-v101_DEV',MAIN,100,101,None),('snapshot14_v100-to-v101_DEV',V14,100,101,None),('snapshot12_v102-to-v103_DEV',V12,102,103,1),('snapshot13_v103-to-v104_bootv2_DEV',V13,103,104,2)]:
            catalog.append(package_one(*args))
        save(DELIVERY/'MCU交付目录.json',catalog)
        print(json.dumps(catalog,ensure_ascii=False))
