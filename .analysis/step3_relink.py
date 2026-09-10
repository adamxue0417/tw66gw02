"""Relink documentation after whole-directory migration; retain edit preimages."""
from pathlib import Path
import hashlib,json,os,re
from urllib.parse import unquote

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'00_项目索引/证据/第三步'
BACKUP=ROOT/'90_历史原始包/整理前备份/20260909-step3/edited-files'
maps=json.loads((OUT/'路径映射.json').read_text(encoding='utf-8'))
edits=[]

def translate(rel,reverse=False):
    key,value=('target','source') if reverse else ('source','target')
    for m in sorted(maps,key=lambda x:len(x[key]),reverse=True):
        if rel==m[key] or rel.startswith(m[key]+'/'):
            return m[value]+rel[len(m[key]):]
    return rel

def save(p,content,reason):
    before=p.read_bytes();after=content.encode('utf-8')
    if before==after:return
    old=translate(p.relative_to(ROOT).as_posix(),True)
    b=BACKUP/old;b.parent.mkdir(parents=True,exist_ok=True)
    if not b.exists():b.write_bytes(before)
    p.write_bytes(after)
    edits.append({'path':p.relative_to(ROOT).as_posix(),'original_path':old,'before_sha256':hashlib.sha256(before).hexdigest(),'after_sha256':hashlib.sha256(after).hexdigest(),'reason':reason})

def main():
    skip={'.git','.git.disabled','.venv','__pycache__','deps','private_keys','.pack','90_历史原始包','.analysis'}
    for d,ds,fs in os.walk(ROOT):
        ds[:]=[x for x in ds if x not in skip]
        for n in fs:
            p=Path(d)/n
            if p.suffix.lower()!='.md':continue
            try:text=p.read_text(encoding='utf-8-sig')
            except UnicodeError:continue
            oldfile=ROOT/translate(p.relative_to(ROOT).as_posix(),True)
            def change(m):
                label,raw=m.group(1),m.group(2);url=raw.strip('<>')
                if url.startswith(('http:','https:','mailto:','#')):return m.group(0)
                path,sep,anchor=url.partition('#')
                original=(oldfile.parent/unquote(path)).resolve()
                if not original.is_relative_to(ROOT):return m.group(0)
                newtarget=ROOT/translate(original.relative_to(ROOT).as_posix())
                current=(p.parent/unquote(path)).resolve()
                if newtarget==current:return m.group(0)
                relative=Path(os.path.relpath(newtarget,p.parent)).as_posix()
                return label+'(<'+relative+(sep+anchor if sep else '')+'>)'
            result=re.sub(r'(!?\[[^\]]*\])\((<[^>]+>|[^\s)]+)\)',change,text)
            if result!=text:save(p,result,'Relocate local documentation links')

    # Keep 13's existing key provider (snapshot 12), independent of drive and workspace name.
    p=ROOT/'01_固件工程/development/snapshot-13_poweroff-v104/tw66gw02/BuildV104Delivery.ps1'
    text=p.read_text(encoding='utf-8-sig')
    old="[string]$KeyRoot = 'C:\\Users\\PC\\Desktop\\TW66GW02\\12\\private_keys'"
    new="[string]$KeyRoot = (Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) 'snapshot-12_secure-ota\\private_keys')"
    assert old in text
    save(p,text.replace(old,new),'Preserve snapshot-12 key provider using relative default')
    p=ROOT/'06_工具与参考/JFlash配置/tw66gw02_snapshot10.jflash'
    text=p.read_text(encoding='utf-8-sig')
    old='C:\\Users\\PC\\Desktop\\TW66GW02\\10\\tw66gw02\\tw66gw02.hex'
    new=str(ROOT/'01_固件工程/history/snapshot-10_development-ota/tw66gw02/tw66gw02.hex')
    assert old in text
    save(p,text.replace(old,new),'Preserve J-Flash image selection for snapshot 10')
    (OUT/'文档与路径修改.json').write_text(json.dumps(edits,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('Edited path/document files:',len(edits))

if __name__=='__main__':main()
