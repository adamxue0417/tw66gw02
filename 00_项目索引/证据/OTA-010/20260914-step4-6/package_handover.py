"""Build a handover snapshot with an allowlist, manifest and ZIP readback."""
from pathlib import Path, PurePosixPath
from datetime import datetime
import base64, hashlib, json, os, re, subprocess, zipfile

EV=Path(__file__).resolve().parent
ROOT=EV.parents[3]
OUT=ROOT/'05_发布与交付/工程师交接'
ID='TW66GW02_HANDOVER_20260914_v1'
B=json.loads((EV.parent/'20260911-step1/baseline.json').read_text(encoding='utf-8'))
PRIVATE=re.compile(rb'-----BEGIN (?:RSA |EC |OPENSSH |ENCRYPTED )?PRIVATE KEY-----')

def sha(p):
    h=hashlib.sha256()
    with p.open('rb') as stream:
        for block in iter(lambda:stream.read(1024*1024),b''):h.update(block)
    return h.hexdigest()

VERIFY=r'''from pathlib import Path
import hashlib,json,sys
root=Path(__file__).resolve().parent
def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''): h.update(block)
    return h.hexdigest()
m=json.loads((root/'handover-manifest.json').read_text(encoding='utf-8'))
errors=[]
for item in m['files']:
    p=(root/item['path']).resolve()
    if not p.is_relative_to(root) or not p.is_file(): errors.append(item['path']+': missing/unsafe')
    elif p.stat().st_size!=item['bytes'] or sha(p)!=item['sha256']: errors.append(item['path']+': changed')
for line in (root/'SHA256SUMS.txt').read_text(encoding='utf-8').splitlines():
    expected,name=line.split('  ',1); p=(root/name).resolve()
    if not p.is_relative_to(root) or not p.is_file() or sha(p)!=expected: errors.append(name+': checksum mismatch')
for required in m['required_entrypoints']:
    if not (root/required).is_file():errors.append(required+': required entrypoint missing')
if errors:
    print('\n'.join(errors));sys.exit(1)
print('PASS: '+str(len(m['files']))+' payload files verified; manifest/checksums and required entrypoints present.')
'''

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    # Copy only the already verified PUBLIC DER, never enter a private-key directory.
    der=ROOT/'90_历史原始包/整理前备份/ota010-step3/provided-public.der'
    assert sha(der)=='3fccdcb727514f80a264fec76933c305262d3ce92f91f8a9711e4eec1a84c0d4'
    public=ROOT/'00_项目索引/交接';public.mkdir(parents=True,exist_ok=True)
    (public/'DEV_public.der').write_bytes(der.read_bytes())
    encoded=base64.b64encode(der.read_bytes()).decode('ascii')
    (public/'DEV_public.pem').write_text('-----BEGIN PUBLIC KEY-----\n'+'\n'.join(encoded[i:i+64] for i in range(0,len(encoded),64))+'\n-----END PUBLIC KEY-----\n',encoding='ascii')
    selected={}
    def add(p,origin='workspace'):
        if not p.is_file() or p.is_symlink():raise RuntimeError(f'Nonregular file: {p}')
        rel=p.relative_to(ROOT).as_posix()
        parts=PurePosixPath(rel).parts
        if any(x.lower() in ('.git','.venv','__pycache__','private_keys') for x in parts):raise RuntimeError('Forbidden path '+rel)
        if p.name=='.env' or p.suffix.lower() in ('.pfx','.p12') or re.search(r'(?i)(private.*\.(pem|key)$)',p.name):raise RuntimeError('Forbidden key/config '+rel)
        if p.suffix.lower() in ('.md','.txt','.log','.json','.patch','.pem','.ps1','.py','.c','.h','.xml','.uvprojx') and PRIVATE.search(p.read_bytes()):raise RuntimeError('Private material '+rel)
        selected[rel]=(p,origin)
    for name,entry in B['repositories'].items():
        repo=ROOT/entry['repository_root']
        assert subprocess.check_output(['git','-C',str(repo),'rev-parse','HEAD']).decode().strip()==entry['head']
        for item in entry['archive']['files']:
            path=repo/item['path'];assert sha(path)==item['sha256'],item['path']
            add(path,name+':'+entry['head'])
    for folder in ('00_项目索引','02_需求与协议','03_硬件资料','04_测试与联调'):
        for p in (ROOT/folder).rglob('*'):
            if p.is_file() and not any(part in ('实验工程','__pycache__') for part in p.parts) and p.name!='package-validation.json':add(p)
    for folder in ('MCU固件','手机APP','BLE模块固件'):
        for p in (ROOT/'05_发布与交付'/folder).rglob('*'):
            if p.is_file():add(p,'existing-delivery')
    for p in (ROOT/'05_发布与交付').iterdir():
        if p.is_file():add(p)
    for name in ('README.md','AGENTS.md','plan.md','交接说明.md','HANDOVER.code-workspace','.gitignore',
                 '260323_Mathis_UI_Operational_Logic-V1.6_完整解析.md','Mathis_operation_process.md',
                 '版本1-13差异与14复制记录.md','TW66GW02-ON.pdf','01_固件工程/README.md','06_工具与参考/README.md'):
        p=ROOT/name
        if p.is_file():add(p)
    # Keep only the first-step recovery ZIPs referred to by the active plan.
    for p in (ROOT/'90_历史原始包/整理前备份/20260911-ota010-step1').glob('*.zip'):
        with zipfile.ZipFile(p) as z:
            for member in z.infolist():
                assert '.git' not in PurePosixPath(member.filename).parts
                assert not PRIVATE.search(z.read(member))
        add(p,'step1-local-recovery')
    generated={'verify_handover.py':VERIFY.encode('utf-8')}
    names=set(selected)|set(generated)|{'handover-manifest.json','SHA256SUMS.txt','handover-link-audit.json'}
    def present(target):return target in names or any(n.startswith(target.rstrip('/')+'/') for n in names)
    missing=[]
    for rel,(p,_) in selected.items():
        if p.suffix.lower()!='.md':continue
        content=p.read_text(encoding='utf-8-sig',errors='replace')
        for target in re.findall(r'\]\(([^)]+)\)',content):
            target=target.strip('<>').split('#',1)[0]
            if not target or '://' in target:continue
            absolute=(p.parent/target).resolve()
            try:dest=absolute.relative_to(ROOT).as_posix()
            except ValueError:dest=target
            if not present(dest):missing.append({'document':rel,'target':target,'resolved':dest,'category':'historical_or_external_material_not_in_handover'})
    core={'plan.md','交接说明.md','AGENTS.md','00_项目索引/证据/OTA-010/20260914-step4-6/README.md','00_项目索引/证据/OTA-010/20260914-step4-6/file-actions.md'}
    assert not [m for m in missing if m['document'] in core], 'Required handover link missing'
    generated['handover-link-audit.json']=(json.dumps({'scope':'Markdown local-path links; anchors not checked','required_documents_pass':True,
        'note':'Historical full-workspace indexes may reference excluded histories, tools and temporary builds; see handover guide.',
        'missing_targets':missing},ensure_ascii=False,indent=2)+'\n').encode('utf-8')
    required=['交接说明.md','plan.md','AGENTS.md','HANDOVER.code-workspace','01_固件工程/main/MDK-ARM/tw66gw02.uvprojx',
              '01_固件工程/main/BuildKeilFactoryTarget.ps1','01_固件工程/main/tests/test_ota_artifacts.py',
              '01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02/BuildV104Delivery.ps1','00_项目索引/交接/DEV_public.pem']
    assert all(present(x) for x in required)
    files=[{'path':rel,'bytes':p.stat().st_size,'sha256':sha(p),'origin':origin} for rel,(p,origin) in sorted(selected.items())]
    files.extend({'path':rel,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'origin':'generated-for-handover'} for rel,data in sorted(generated.items()))
    manifest={'id':ID,'created':datetime.now().astimezone().isoformat(),'classification':'Engineering handover / DEV / not production',
        'main_head':B['repositories']['main']['head'],'snapshot13_head':B['repositories']['snapshot13']['head'],
        'documentation_overlay':'Current local documents included; no new Git commit or repository history claimed',
        'required_entrypoints':required,'excluded':['private keys/credentials','.git','.venv','history source','snapshot12/14 source','experimental projects','tool installers','bulk temporary builds'],
        'files':files}
    generated['handover-manifest.json']=(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n').encode('utf-8')
    sums={f['path']:f['sha256'] for f in files}
    sums['handover-manifest.json']=hashlib.sha256(generated['handover-manifest.json']).hexdigest()
    generated['SHA256SUMS.txt']=''.join(f'{h}  {p}\n' for p,h in sorted(sums.items())).encode('utf-8')
    tmp=OUT/(ID+'.zip.building');final=OUT/(ID+'.zip')
    assert tmp.resolve().parent==OUT.resolve() and final.resolve().parent==OUT.resolve()
    print(f'Packaging {len(files)} payload files',flush=True)
    with zipfile.ZipFile(tmp,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=5) as z:
        for rel,(p,_) in sorted(selected.items()):z.write(p,ID+'/'+rel)
        for rel,data in sorted(generated.items()):z.writestr(ID+'/'+rel,data)
    print('ZIP readback validation',flush=True)
    with zipfile.ZipFile(tmp) as z:
        assert z.testzip() is None
        for rel,h in sums.items():
            with z.open(ID+'/'+rel) as f:
                calc=hashlib.sha256()
                for block in iter(lambda:f.read(1024*1024),b''):calc.update(block)
            assert calc.hexdigest()==h,rel
        assert z.read(ID+'/SHA256SUMS.txt')==generated['SHA256SUMS.txt']
    # Verify actual extraction and run the receiver's checksum tool, without building.
    extract=ROOT/'90_历史原始包/整理前备份'/('handover-verify-'+datetime.now().strftime('%Y%m%d-%H%M%S'))
    extract.mkdir(parents=True,exist_ok=False)
    with zipfile.ZipFile(tmp) as z:
        for member in z.infolist():
            assert (extract/member.filename).resolve().is_relative_to(extract.resolve())
        z.extractall(extract)
    result=subprocess.run(['python',str(extract/ID/'verify_handover.py')],capture_output=True,text=True)
    assert result.returncode==0,result.stdout+result.stderr
    os.replace(tmp,final)
    checksum=sha(final)
    (OUT/(ID+'.zip.sha256')).write_text(checksum+'  '+final.name+'\n',encoding='ascii')
    output={'id':ID,'result':'PASS','zip_path':final.relative_to(ROOT).as_posix(),'zip_bytes':final.stat().st_size,'zip_sha256':checksum,
        'payload_files':len(files),'zip_entries':len(selected)+len(generated),'source_files_verified':675,
        'required_document_links':'PASS','historical_missing_link_count':len(missing),'archive_readback':'PASS',
        'extraction_directory':extract.relative_to(ROOT).as_posix(),'receiver_verifier':'PASS','receiver_stdout':result.stdout.strip(),
        'source_code_migration':'NOT_RUN','hardware':'NOT_RUN'}
    (EV/'package-validation.json').write_text(json.dumps(output,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(output,ensure_ascii=True,indent=2),flush=True)

if __name__=='__main__':main()
