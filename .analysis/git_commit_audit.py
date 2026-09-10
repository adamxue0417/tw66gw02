"""Read-only staged tree audit before publishing workspace repositories."""
import pathlib, re, subprocess, sys, json
repo=pathlib.Path(sys.argv[1]).resolve()
raw=subprocess.check_output(['git','-C',str(repo),'ls-files','--stage','-z'])
issues=[];count=0;total=0
for entry in raw.split(b'\0'):
    if not entry:continue
    meta,name=entry.split(b'\t',1);mode,oid,stage=meta.split();path=name.decode('utf-8')
    if mode==b'160000':issues.append({'path':path,'reason':'unexpected embedded repository'});continue
    data=subprocess.check_output(['git','-C',str(repo),'cat-file','blob',oid.decode()])
    count+=1;total+=len(data)
    if len(data)>100*1024*1024:issues.append({'path':path,'reason':'blob exceeds 100 MiB'})
    if any(part.lower() in {'.venv','private_keys','.git.disabled','__pycache__'} for part in pathlib.PurePosixPath(path).parts):issues.append({'path':path,'reason':'excluded local directory'})
    if re.search(rb'(?m)^-----BEGIN (?:RSA |EC |OPENSSH |ENCRYPTED )?PRIVATE KEY-----\r?$',data):issues.append({'path':path,'reason':'private key header'})
    if re.search(rb'\b(?:ghp_[A-Za-z0-9]{30,}|github_pat_[A-Za-z0-9_]{50,})\b',data):issues.append({'path':path,'reason':'credential-like token'})
print(json.dumps({'repo':str(repo),'index_files':count,'index_bytes':total,'issues':issues},ensure_ascii=True))
sys.exit(bool(issues))
