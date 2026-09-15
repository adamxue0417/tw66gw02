"""Check v104-specific factory composition and the compiled public key."""
from pathlib import Path
import hashlib, importlib.util, json, re, struct, subprocess, zlib

EV=Path(__file__).resolve().parent
ROOT=EV.parents[3]
LOCAL=ROOT/'90_历史原始包/整理前备份/ota010-step3'
PROJECT=LOCAL/'v104'
ART=PROJECT/'OTA_Artifacts'
def sha(data): return hashlib.sha256(data).hexdigest()

manifest=json.loads((ART/'mathis_ota_v104_dev_signed.manifest.json').read_text(encoding='utf-8-sig'))
app=(ART/'mathis_app_v104.bin').read_bytes()
ota=(ART/'mathis_ota_v104_dev_signed.ota').read_bytes()
old=(ART/'mathis_app_v103.bin').read_bytes()
boot=(ART/'mathis_secure_bootloader_dev_v2.bin').read_bytes()
factory=(ART/'mathis_secure_factory_v103_bootv2_dev.bin').read_bytes()
assert manifest['target_version']==manifest['security_version']==104
assert len(app)==manifest['application_size']<=0x6280
assert len(ota)==manifest['artifact_size']==len(app)+384
assert ota[:-384]==app and ota[-384:]!=bytes(384)
assert sha(app)==manifest['application_sha256'].lower()
assert sha(ota)==manifest['artifact_sha256'].lower()
assert zlib.crc32(ota)==int(manifest['crc32_iso_hdlc'],16)
assert len(boot)<=8192 and len(factory)==8192+len(old)
assert factory[:len(boot)]==boot and factory[len(boot):8192]==bytes([255])*(8192-len(boot))
assert factory[8192:]==old
assert sha(old)=='c3ad66c2302aca9cfcc19bfd8e77197eb7204ca1fe13db7cfa4fe37f7869acae'
for image in (old,app):
    sp,reset=struct.unpack_from('<II',image)
    assert 0x200000C0<=sp<=0x20001FE0 and 0x08002000<=(reset&~1)<0x08002000+len(image)
magic,abi,size,cap,layout,verify,floor=struct.unpack_from('<IHHIIII',boot,0x1FC0)
assert (magic,abi,size,cap,layout)==(0x4950414D,1,60,5,2)
assert all(p&1 and 0x08000001<=p<0x08001FC0 for p in (verify,floor))
fingerprint=sha((LOCAL/'provided-public.der').read_bytes())
assert boot[0x1FD8:0x1FF8].hex()==fingerprint==manifest['signing_key_fingerprint_sha256'].lower()

source=(PROJECT/'Bootloader/boot_security.c').read_text(encoding='utf-8-sig')
def values(name):
    body=re.search(r'\b'+name+r'\[[^]]+\]\s*=\s*\{(.*?)\};',source,re.S).group(1)
    return [int(x,16) for x in re.findall(r'0x([0-9A-Fa-f]+)u?',body)]
raw=bytes(values('s_dev_modulus'))
limbs=values('s_dev_modulus_i15')
number=sum(value<<(15*i) for i,value in enumerate(limbs[1:]))
assert number.bit_length()==3072 and number==int.from_bytes(raw,'big')
modulus=subprocess.check_output(['C:/Program Files/Git/usr/bin/openssl.exe','rsa','-pubin','-inform','DER','-in',str(LOCAL/'provided-public.der'),'-modulus','-noout']).decode().strip().split('=',1)[1]
assert number==int(modulus,16)
assert struct.pack('<'+'H'*len(limbs),*limbs) in boot
spec=importlib.util.spec_from_file_location('existing_artifact_checks',PROJECT/'tests/test_ota_artifacts.py')
module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
hexdata=module.parse_hex(ART/'mathis_secure_factory_v103_bootv2_dev.hex')
assert min(hexdata)==0x08000000 and max(hexdata)<0x08008400
assert all(factory[address-0x08000000]==value for address,value in hexdata.items())
frozen=ROOT/'05_发布与交付/MCU固件/snapshot13_v103-to-v104_bootv2_DEV/App_Upload/mathis_ota_v104_dev_signed.ota'
report={'result':'PASS','application_size':len(app),'artifact_size':len(ota),'bootloader_size':len(boot),'factory_size':len(factory),
        'application_sha256':sha(app),'artifact_sha256':sha(ota),'crc32':manifest['crc32_iso_hdlc'],
        'bootloader_sha256':sha(boot),'factory_sha256':sha(factory),'matches_frozen_v104':sha(ota)==sha(frozen.read_bytes()),
        'boot_api':{'abi':abi,'size':size,'capabilities':cap,'layout':layout,'prod_key_enabled':False},
        'compiled_i15_modulus_matches_public_key':True,'key_bits':number.bit_length(),'public_fingerprint_sha256':fingerprint,
        'factory_bin_and_hex_match':True,'original_v103_input_preserved':True,'hardware':'NOT_RUN'}
(EV/'v104-validation.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,indent=2))
