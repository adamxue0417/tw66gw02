"""Isolated snapshot-13 builds; DEV key referenced in place, never archived.

--no-sign skips private-key use while still building and running offline checks.
"""
from pathlib import Path
from datetime import datetime
import hashlib, json, re, shutil, struct, subprocess, sys, zipfile

EV = Path(__file__).resolve().parent
ROOT = EV.parents[3]
LOCAL = ROOT / '90_历史原始包/整理前备份/ota010-step3'
KEYS = ROOT / '01_固件工程/development/snapshot-12_secure-ota/private_keys'
OPENSSL = 'C:/Program Files/Git/usr/bin/openssl.exe'
PS = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File']

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def stamp(): return datetime.now().astimezone().isoformat()
report = {'task': 'OTA-010', 'step': 3, 'started': stamp(), 'commands': {}, 'hardware': 'NOT_RUN'}
def save(): (EV/'result.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')

def run(name, cmd, cwd):
    print('RUN '+name, flush=True)
    log = EV/(name+'.log')
    with log.open('wb') as stream:
        try:
            result = subprocess.run([str(x) for x in cmd], cwd=cwd, stdout=stream, stderr=subprocess.STDOUT, timeout=300)
            code = result.returncode
        except subprocess.TimeoutExpired:
            code = -1
            stream.write(b'\nRunner timeout after 300 seconds.\n')
    report['commands'][name] = {'command':[str(x) for x in cmd], 'cwd':str(cwd),
                               'exit_code':code, 'log':log.name, 'sha256':sha(log)}
    save(); print(f'END {name}: {code}',flush=True)
    return code == 0

def source_state(baseline):
    n=0
    for data in baseline['repositories'].values():
        repo=ROOT/data['repository_root']
        assert subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD']).decode().strip()==data['head']
        for f in data['archive']['files']:
            assert sha(repo/f['path'])==f['sha256'],f['path']; n+=1
    return n

def prepare(name, archive, files):
    path=LOCAL/name; path.mkdir(parents=True)
    selected=[]
    with zipfile.ZipFile(archive) as z:
        for f in files:
            rel=f['path']; parts=Path(rel).parts
            if parts[0] in ('OTA_Artifacts','App_Delivery'): continue
            if Path(rel).suffix.lower() in ('.bin','.hex','.axf','.map','.htm','.log'): continue
            p=path/rel
            assert p.resolve().is_relative_to(path.resolve())
            p.parent.mkdir(parents=True,exist_ok=True); p.write_bytes(z.read(rel))
            assert sha(p)==f['sha256']; selected.append(f)
    report.setdefault('projects',{})[name]={'path':str(path),'source_files':selected,'retained_inputs':[]}
    return path

def main():
    if LOCAL.exists() or (EV/'result.json').exists(): raise RuntimeError('Do not overwrite an existing run')
    baseline=json.loads((EV.parent/'20260911-step1/baseline.json').read_text(encoding='utf-8'))
    entry=baseline['repositories']['snapshot13']; archive=ROOT/entry['archive']['path']
    assert sha(archive)==entry['archive']['sha256']
    report['source_head']=entry['head']; report['source_archive_sha256']=sha(archive)
    report['original_source_count_before']=source_state(baseline)
    delivery=ROOT/'05_发布与交付/MCU固件'
    frozen={p.relative_to(ROOT).as_posix():sha(p) for folder in ('main_v100-to-v101_DEV','snapshot13_v103-to-v104_bootv2_DEV') for p in (delivery/folder).rglob('*') if p.is_file()}
    report['frozen_before']=frozen
    v104=prepare('v104',archive,entry['archive']['files'])
    compat=prepare('compat-v103',archive,entry['archive']['files'])
    # Exact historical v103 image is an explicit BuildV104Delivery dependency.
    with zipfile.ZipFile(archive) as z:
        for rel in ('OTA_Artifacts/mathis_app_v103.bin','OTA_Artifacts/mathis_app_v103.hex'):
            target=v104/rel; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(z.read(rel))
            report['projects']['v104']['retained_inputs'].append({'path':rel,'sha256':sha(target)})
    assert sha(v104/'OTA_Artifacts/mathis_app_v103.bin')=='c3ad66c2302aca9cfcc19bfd8e77197eb7204ca1fe13db7cfa4fe37f7869acae'
    save()
    try:
        run('compiler-version',['C:/Keil_v5/ARM/ARM_Compiler_5.06u7/Bin/armcc.exe','--vsn'],LOCAL)
        run('openssl-version',[OPENSSL,'version'],LOCAL)
        # Only public DER is written; stdout/stderr never contain private key bytes.
        key_ready=False
        if '--no-sign' not in sys.argv:
            a=run('derive-dev-public',[OPENSSL,'pkey','-in',KEYS/'mathis_dev_rsa3072_private.pem','-pubout','-outform','DER','-out',LOCAL/'derived-public.der'],LOCAL)
            b=run('normalize-dev-public',[OPENSSL,'pkey','-pubin','-in',KEYS/'mathis_dev_rsa3072_public.pem','-pubout','-outform','DER','-out',LOCAL/'provided-public.der'],LOCAL)
            if a and b:
                expected='3fccdcb727514f80a264fec76933c305262d3ce92f91f8a9711e4eec1a84c0d4'
                key_ready=sha(LOCAL/'derived-public.der')==sha(LOCAL/'provided-public.der')==sha(KEYS/'mathis_dev_rsa3072_public.der')==expected
                report['dev_public_fingerprint_sha256']=sha(LOCAL/'derived-public.der')
        report['dev_key_preflight']='PASS' if key_ready else 'BLOCKED'; save()
        if key_ready:
            run('v104-delivery',PS+[v104/'BuildV104Delivery.ps1','-KeyRoot',KEYS],v104)
            run('compat-v102-v103-build',PS+[compat/'BuildOtaArtifacts.ps1','-FactoryVersion','102','-OtaVersion','103',
                '-DevPrivateKey',KEYS/'mathis_dev_rsa3072_private.pem','-DevPublicPem',KEYS/'mathis_dev_rsa3072_public.pem',
                '-DevPublicDer',KEYS/'mathis_dev_rsa3072_public.der'],compat)
            if (compat/'OTA_Artifacts/mathis_ota_v103_dev_signed.ota').exists():
                run('secure-artifacts', [sys.executable,compat/'tests/test_ota_artifacts.py'],compat)
                run('signature-v103',PS+[compat/'tests/verify_signed_ota.ps1','-PublicKey',KEYS/'mathis_dev_rsa3072_public.pem'],compat)
            if (v104/'OTA_Artifacts/mathis_ota_v104_dev_signed.ota').exists():
                verifier=v104/'tests/verify_signed_v104.ps1'
                verifier.write_text((v104/'tests/verify_signed_ota.ps1').read_text(encoding='utf-8-sig').replace('v103','v104'),encoding='utf-8-sig')
                report['v104_signature_test_adapter']='Existing verifier copied with v103 artifact/manifest names replaced by v104; source verifier unchanged'
                run('signature-v104',PS+[verifier,'-PublicKey',KEYS/'mathis_dev_rsa3072_public.pem'],v104)
        else:
            run('v104-bootloader',PS+[v104/'Bootloader/build_bootloader.ps1','-OutputDir',v104/'Bootloader/build_v2'],v104)
            for version in (102,103,104):
                project=v104 if version==104 else compat
                run(f'unsigned-v{version}',PS+[EV/'build_unsigned.ps1','-ProjectRoot',project,'-Version',str(version)],project)
        run('protocol-tests',[sys.executable,'-m','unittest','discover','-s',str(v104/'Tools/PC_BLE_Test/tests'),'-v'],v104)
        run('pc-ble-selftest',[sys.executable,v104/'Tools/PC_BLE_Test/mathis_ble.py','selftest'],v104)
        report['result']='PASS' if key_ready and all(c['exit_code']==0 for c in report['commands'].values()) else ('BLOCKED' if not key_ready else 'FAIL')
    finally:
        report['original_source_count_after']=source_state(baseline)
        assert all(sha(ROOT/name)==value for name,value in frozen.items())
        report['frozen_files_unchanged']=len(frozen)
        for name,data in report['projects'].items():
            project=Path(data['path'])
            data['source_files_changed']=[f['path'] for f in data['source_files'] if not (project/f['path']).exists() or sha(project/f['path'])!=f['sha256']]
            data['outputs']=[]
            for directory in ('OTA_Artifacts','Bootloader/build','Bootloader/build_v2'):
                for p in sorted((project/directory).rglob('*')):
                    if p.is_file() and p.suffix.lower() in ('.bin','.hex','.ota','.json','.map','.htm','.log'):
                        item={'path':p.relative_to(project).as_posix(),'bytes':p.stat().st_size,'sha256':sha(p)}
                        data['outputs'].append(item)
                        if p.suffix=='.log' or p.name in ('mathis_bootloader.map','mathis_bootloader.htm'):
                            output=EV/(name+'-'+p.name); shutil.copyfile(p,output)
            if data['source_files_changed']: report['result']='FAIL'
        report['finished']=stamp(); save()
        print('RESULT '+report.get('result','INCOMPLETE'),flush=True)
        print('Original source and frozen deliveries unchanged.',flush=True)

if __name__=='__main__': main()
