from __future__ import annotations
import binascii, hashlib, json, struct
from dataclasses import dataclass
from pathlib import Path

START=b"\x5a\x5a"; MAX_PAYLOAD=236
TYPE_CMD=0x01; TYPE_TELEMETRY=0x81; TYPE_OTA_BEGIN=0x10; TYPE_OTA_CHUNK=0x11
TYPE_OTA_COMMIT=0x12; TYPE_OTA_ABORT=0x13; TYPE_OTA_STATUS=0x90
CMD_POWER_OFF=0x02; CMD_TELEMETRY_CTRL=0xFF; TELEMETRY_STOP=0; TELEMETRY_START=1
STATUS_READY=1; STATUS_ACK=3; STATUS_VERIFY_OK=4; STATUS_VERIFY_FAILED=5
STATUS_APPLYING=6; STATUS_ABORTED=7; STATUS_POWER=8; STATUS_STORAGE=9
STATUS_BUSY=10; STATUS_BAD_OFFSET=11
STATUS_NAMES={1:"READY",3:"ACK",4:"VERIFY_OK",5:"VERIFY_FAILED",6:"APPLYING",7:"ABORTED",8:"POWER",9:"STORAGE",10:"BUSY",11:"BAD_OFFSET"}
REASON_NAMES={1:"CRC",2:"SIGNATURE",3:"SIZE",4:"TIMEOUT",5:"ROLLBACK",255:"UNKNOWN"}

def crc16_ccitt(data:bytes)->int:
    crc=0xFFFF
    for value in data:
        crc ^= value<<8
        for _ in range(8): crc=((crc<<1)^0x1021)&0xFFFF if crc&0x8000 else (crc<<1)&0xFFFF
    return crc

def crc32_iso_hdlc(data:bytes)->int: return binascii.crc32(data)&0xFFFFFFFF

@dataclass(frozen=True)
class Frame:
    type:int; sequence:int; payload:bytes; raw:bytes

def encode_frame(frame_type:int, sequence:int, payload:bytes=b"")->bytes:
    if len(payload)>MAX_PAYLOAD: raise ValueError("payload too large")
    body=bytes((frame_type&255,sequence&255))+struct.pack("<H",len(payload))+payload
    return START+body+struct.pack("<H",crc16_ccitt(body))

def decode_frame(raw:bytes)->Frame:
    if len(raw)<8 or raw[:2]!=START: raise ValueError("bad header")
    size=struct.unpack_from("<H",raw,4)[0]
    if size>MAX_PAYLOAD or len(raw)!=size+8: raise ValueError("bad length")
    if struct.unpack_from("<H",raw,len(raw)-2)[0]!=crc16_ccitt(raw[2:-2]): raise ValueError("bad CRC16")
    return Frame(raw[2],raw[3],raw[6:-2],raw)

class FrameParser:
    def __init__(self): self.buffer=bytearray()
    def feed(self,chunk:bytes)->list[Frame]:
        self.buffer.extend(chunk); out=[]
        while True:
            start=self.buffer.find(START)
            if start<0:
                self.buffer[:]=self.buffer[-1:] if self.buffer[-1:]==b"\x5a" else b""; break
            if start: del self.buffer[:start]
            if len(self.buffer)<6: break
            size=struct.unpack_from("<H",self.buffer,4)[0]
            if size>MAX_PAYLOAD: del self.buffer[0]; continue
            total=size+8
            if len(self.buffer)<total: break
            try: out.append(decode_frame(bytes(self.buffer[:total]))); del self.buffer[:total]
            except ValueError: del self.buffer[0]
        return out

def parse_telemetry(frame:Frame)->dict:
    if frame.type!=TYPE_TELEMETRY or len(frame.payload)!=12: raise ValueError("not telemetry")
    mode=frame.payload[0]
    return {"mode_bits":mode,"valid_mask":mode&15,"units":"C" if mode&16 else "F",
            "temperatures":struct.unpack_from("<hhhh",frame.payload,1),
            "battery_percent":frame.payload[9],"firmware_version":frame.payload[10],"error_code":frame.payload[11]}

def parse_status(frame:Frame)->dict:
    if frame.type!=TYPE_OTA_STATUS or not frame.payload: raise ValueError("not OTA status")
    code=frame.payload[0]; result={"code":code,"name":STATUS_NAMES.get(code,f"0x{code:02X}"),"extra":frame.payload[1:]}
    if code==STATUS_READY and len(frame.payload)==3: result["chunk_size"]=struct.unpack_from("<H",frame.payload,1)[0]
    elif code==STATUS_ACK and len(frame.payload)==5: result["next_offset"]=struct.unpack_from("<I",frame.payload,1)[0]
    elif code in (STATUS_VERIFY_FAILED,STATUS_ABORTED) and len(frame.payload)>=2:
        result["reason"]=frame.payload[1]; result["reason_name"]=REASON_NAMES.get(frame.payload[1],f"0x{frame.payload[1]:02X}")
    return result

def load_and_validate_artifact(manifest_path:Path,ota_path:Path)->tuple[dict,bytes]:
    manifest=json.loads(manifest_path.read_text(encoding="utf-8-sig")); artifact=ota_path.read_bytes()
    required={"target_version","security_version","application_size","signature_size","artifact_size","crc32_iso_hdlc","application_sha256","artifact_sha256","signature_algorithm","signing_key_class","development_signature_placeholder"}
    missing=required.difference(manifest)
    if missing: raise ValueError(f"manifest missing {sorted(missing)}")
    if manifest["target_version"]!=104 or manifest["security_version"]!=104: raise ValueError("version is not 104")
    if manifest["signing_key_class"]!="DEV" or manifest["development_signature_placeholder"] is not False: raise ValueError("not real DEV signature")
    if manifest["signature_algorithm"]!="RSA-3072-PKCS1-v1_5-SHA256" or manifest["signature_size"]!=384: raise ValueError("bad signature format")
    if manifest["artifact_size"]!=len(artifact) or manifest["application_size"]+384!=len(artifact): raise ValueError("size mismatch")
    if crc32_iso_hdlc(artifact)!=int(str(manifest["crc32_iso_hdlc"]),0): raise ValueError("CRC32 mismatch")
    if hashlib.sha256(artifact).hexdigest().upper()!=str(manifest["artifact_sha256"]).upper(): raise ValueError("OTA SHA mismatch")
    app=artifact[:manifest["application_size"]]
    if hashlib.sha256(app).hexdigest().upper()!=str(manifest["application_sha256"]).upper(): raise ValueError("app SHA mismatch")
    return manifest,artifact
