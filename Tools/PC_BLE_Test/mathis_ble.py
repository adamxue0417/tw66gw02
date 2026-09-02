from __future__ import annotations
import argparse, asyncio, json, struct, sys, time
from datetime import datetime, timezone
from pathlib import Path
from protocol import *

DEFAULT_NAME="337910"; DEFAULT_ADDRESS="04:78:63:33:79:10"
ROOT=Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST=ROOT/"OTA_Artifacts"/"mathis_ota_v104_dev_signed.manifest.json"
DEFAULT_OTA=ROOT/"OTA_Artifacts"/"mathis_ota_v104_dev_signed.ota"

class EventLog:
    def __init__(self,command,log_dir):
        log_dir.mkdir(parents=True,exist_ok=True); stamp=datetime.now().strftime("%Y%m%d_%H%M%S")
        self.path=log_dir/f"{command}_{stamp}.jsonl"
    def emit(self,event,**fields):
        record={"time":datetime.now(timezone.utc).isoformat(),"event":event,**fields}
        with self.path.open("a",encoding="utf-8") as f: f.write(json.dumps(record,ensure_ascii=False,default=str)+"\n")
        detail=" ".join(f"{k}={v}" for k,v in fields.items() if k!="raw")
        print(f"[{record['time']}] {event}{(' '+detail) if detail else ''}")

def norm(value): return (value or "").replace("-",":").upper()
def uuid16(uuid,short):
    value=uuid.lower(); short=short.lower()
    return value==short or value.startswith(f"0000{short}-") or value.endswith(short)

async def scan_target(address,name,timeout,log):
    from bleak import BleakScanner
    found=await BleakScanner.discover(timeout=timeout,return_adv=True)
    pairs=list(found.values()) if isinstance(found,dict) else [(d,None) for d in found]
    for device,adv in pairs:
        seen=getattr(adv,"local_name",None) or getattr(device,"name",None)
        log.emit("scan",address=getattr(device,"address",""),name=seen)
        if (address and norm(getattr(device,"address",""))==norm(address)) or (name and seen==name): return device
    return None

def describe(services,log):
    for service in services:
        log.emit("service",uuid=service.uuid,description=getattr(service,"description",""))
        for char in service.characteristics:
            log.emit("characteristic",uuid=char.uuid,properties=list(char.properties),service_uuid=service.uuid,
                     max_write_without_response=getattr(char,"max_write_without_response_size",None))

def select_chars(services,write_uuid,notify_uuid):
    chars=[c for s in services for c in s.characteristics]
    write=next((c for c in chars if c.uuid.lower()==write_uuid.lower()),None) if write_uuid else next((c for c in chars if uuid16(c.uuid,"ffe1") and ("write" in c.properties or "write-without-response" in c.properties)),None)
    notify=next((c for c in chars if c.uuid.lower()==notify_uuid.lower()),None) if notify_uuid else next((c for c in chars if uuid16(c.uuid,"ffe2") and ("notify" in c.properties or "indicate" in c.properties)),None)
    if not write or not notify: raise RuntimeError("未找到FFE1写特征/FFE2 Notify特征；请根据上方GATT表显式指定--write-uuid和--notify-uuid")
    return write,notify

class Link:
    def __init__(self,args,log):
        self.args=args; self.log=log; self.client=None; self.write_char=None; self.notify_char=None
        self.parser=FrameParser(); self.frames=asyncio.Queue(); self.disconnected=asyncio.Event(); self.sequence=0
    async def connect(self,scan_timeout=10.0):
        from bleak import BleakClient
        device=await scan_target(self.args.address,self.args.name,scan_timeout,self.log)
        if device is None: raise RuntimeError("device not found")
        self.client=BleakClient(device,disconnected_callback=lambda _c:self.disconnected.set())
        await self.client.connect(); services=self.client.services; describe(services,self.log)
        self.write_char,self.notify_char=select_chars(services,self.args.write_uuid,self.args.notify_uuid)
        await self.client.start_notify(self.notify_char,self._notify)
        self.log.emit("connected",address=getattr(device,"address",""),write_uuid=self.write_char.uuid,notify_uuid=self.notify_char.uuid)
    def _notify(self,_sender,data):
        raw=bytes(data); self.log.emit("notify",raw=raw.hex().upper(),length=len(raw))
        for frame in self.parser.feed(raw):
            self.log.emit("frame",type=f"0x{frame.type:02X}",sequence=frame.sequence,raw=frame.raw.hex().upper())
            self.frames.put_nowait(frame)
    async def close(self):
        if self.client is not None and self.client.is_connected:
            try: await self.client.stop_notify(self.notify_char)
            except Exception: pass
            await self.client.disconnect()
    async def send(self,kind,payload=b""):
        raw=encode_frame(kind,self.sequence,payload); self.sequence=(self.sequence+1)&255
        capability=int(getattr(self.write_char,"max_write_without_response_size",20) or 20)
        segment=max(1,min(self.args.segment_size,capability)); response="write-without-response" not in self.write_char.properties
        for offset in range(0,len(raw),segment):
            part=raw[offset:offset+segment]
            await self.client.write_gatt_char(self.write_char,part,response=response)
            self.log.emit("write",type=f"0x{kind:02X}",offset=offset,length=len(part),raw=part.hex().upper())
            if offset+segment<len(raw): await asyncio.sleep(max(.025,self.args.gap_ms/1000))
    async def wait_frame(self,wanted,timeout):
        deadline=asyncio.get_running_loop().time()+timeout
        while True:
            remain=deadline-asyncio.get_running_loop().time()
            if remain<=0: raise TimeoutError("frame timeout")
            frame=await asyncio.wait_for(self.frames.get(),remain)
            if frame.type in wanted: return frame
    async def wait_status(self,timeout):
        status=parse_status(await self.wait_frame({TYPE_OTA_STATUS},timeout))
        self.log.emit("ota_status",**{k:(v.hex().upper() if isinstance(v,bytes) else v) for k,v in status.items()})
        return status

async def cmd_discover(args,log):
    link=Link(args,log)
    try: await link.connect()
    finally: await link.close()

async def cmd_monitor(args,log):
    link=Link(args,log); count=0
    try:
        await link.connect(); await link.send(TYPE_CMD,bytes((CMD_TELEMETRY_CTRL,TELEMETRY_START)))
        deadline=asyncio.get_running_loop().time()+args.seconds
        while asyncio.get_running_loop().time()<deadline:
            telemetry=parse_telemetry(await link.wait_frame({TYPE_TELEMETRY},max(1,deadline-asyncio.get_running_loop().time())))
            log.emit("telemetry",**telemetry); count+=1
        if count==0: raise RuntimeError("no telemetry")
    finally:
        if link.client is not None and link.client.is_connected:
            try: await link.send(TYPE_CMD,bytes((CMD_TELEMETRY_CTRL,TELEMETRY_STOP)))
            except Exception: pass
        await link.close()

async def cmd_poweroff(args,log):
    if not args.confirm_poweroff: raise RuntimeError("add --confirm-poweroff")
    link=Link(args,log); await link.connect(); await link.send(TYPE_CMD,bytes((CMD_POWER_OFF,)))
    try: await asyncio.wait_for(link.disconnected.wait(),3)
    except TimeoutError: await link.close(); raise RuntimeError("BLE did not disconnect within 3 seconds")
    log.emit("poweroff_disconnected")
    if await scan_target(args.address,args.name,30,log) is not None: raise RuntimeError("30秒内重新广播：怀疑PB3重锁存/复位循环")
    log.emit("poweroff_pass",battery_only=True,quiet_seconds=30)

FAILURES={STATUS_VERIFY_FAILED,STATUS_ABORTED,STATUS_POWER,STATUS_STORAGE,STATUS_BUSY,STATUS_BAD_OFFSET}
async def reconnect_stable(args,log,version):
    deadline=time.monotonic()+120; first=None; consecutive=0; reconnects=0
    while time.monotonic()<deadline:
        link=Link(args,log)
        try:
            await link.connect(min(10,max(1,deadline-time.monotonic()))); reconnects+=1
            await link.send(TYPE_CMD,bytes((CMD_TELEMETRY_CTRL,TELEMETRY_START)))
            while time.monotonic()<deadline and not link.disconnected.is_set():
                try: telemetry=parse_telemetry(await link.wait_frame({TYPE_TELEMETRY},3))
                except TimeoutError: continue
                log.emit("reconnect_telemetry",reconnect=reconnects,**telemetry)
                if telemetry["firmware_version"]==version:
                    first=first or time.monotonic(); consecutive+=1
                    if time.monotonic()-first>=15 and consecutive>=2:
                        log.emit("ota_final",version=version,reconnects=reconnects); return
                else: consecutive=0
        except Exception as exc: log.emit("reconnect_retry",error=str(exc))
        finally: await link.close()
        await asyncio.sleep(1)
    raise RuntimeError("120秒内未跨越两次重启稳定收到v104遥测")

async def cmd_ota(args,log):
    if args.confirm!="DEV-v104": raise RuntimeError("真实升级必须添加 --confirm DEV-v104")
    manifest,artifact=load_and_validate_artifact(Path(args.manifest),Path(args.ota_file)); crc=int(str(manifest["crc32_iso_hdlc"]),0)
    log.emit("artifact_valid",size=len(artifact),crc=f"0x{crc:08X}",sha256=manifest["artifact_sha256"],version=104,key_class="DEV")
    link=Link(args,log); committed=False
    try:
        await link.connect(); await link.send(TYPE_OTA_BEGIN,struct.pack("<IIB",len(artifact),crc,104))
        ready=await link.wait_status(15)
        if ready["code"]!=STATUS_READY: raise RuntimeError(f"BEGIN rejected: {ready}")
        chunk_size=min(int(ready.get("chunk_size",0)),232)
        if chunk_size<=0: raise RuntimeError("invalid chunk size")
        offset=0
        while offset<len(artifact):
            data=artifact[offset:offset+chunk_size]; payload=struct.pack("<I",offset)+data; accepted=False
            for attempt in range(2):
                await link.send(TYPE_OTA_CHUNK,payload)
                try: status=await link.wait_status(5)
                except TimeoutError:
                    if attempt==0: log.emit("chunk_retry",offset=offset); continue
                    raise
                if status["code"]==STATUS_ACK and status.get("next_offset")==offset+len(data):
                    offset=status["next_offset"]; accepted=True; break
                if status["code"] in FAILURES: raise RuntimeError(f"chunk rejected {offset}: {status}")
                raise RuntimeError(f"unexpected status: {status}")
            if not accepted: raise RuntimeError(f"no ACK at {offset}")
        await link.send(TYPE_OTA_COMMIT,struct.pack("<I",crc)); committed=True; verified=False; deadline=time.monotonic()+60
        while time.monotonic()<deadline:
            status=await link.wait_status(max(.1,deadline-time.monotonic()))
            if status["code"]==STATUS_VERIFY_OK: verified=True
            elif status["code"]==STATUS_APPLYING and verified: break
            elif status["code"] in FAILURES: raise RuntimeError(f"COMMIT rejected: {status}")
        else: raise RuntimeError("60秒内未收到VERIFY_OK→APPLYING")
    except (KeyboardInterrupt,asyncio.CancelledError):
        if not committed and link.client is not None and link.client.is_connected:
            try: await link.send(TYPE_OTA_ABORT); log.emit("abort_sent")
            except Exception as exc: log.emit("abort_failed",error=str(exc))
        else: log.emit("commit_already_sent_wait_for_device")
        raise
    finally: await link.close()
    await reconnect_stable(args,log,104)

def selftest():
    assert crc16_ccitt(b"123456789")==0x29B1 and crc32_iso_hdlc(b"123456789")==0xCBF43926
    raw=encode_frame(TYPE_CMD,7,b"\xff\x01"); p=FrameParser(); frames=p.feed(raw[:3])+p.feed(raw[3:]+raw)
    assert len(frames)==2 and frames[0].payload==b"\xff\x01"; print("selftest PASS")

def parser():
    p=argparse.ArgumentParser(); p.add_argument("--address",default=DEFAULT_ADDRESS); p.add_argument("--name",default=DEFAULT_NAME)
    p.add_argument("--write-uuid"); p.add_argument("--notify-uuid"); p.add_argument("--segment-size",type=int,default=20)
    p.add_argument("--gap-ms",type=float,default=25); p.add_argument("--log-dir",default=str(Path(__file__).parent/"logs"))
    sub=p.add_subparsers(dest="command",required=True); sub.add_parser("selftest"); sub.add_parser("discover")
    m=sub.add_parser("monitor"); m.add_argument("--seconds",type=float,default=10)
    off=sub.add_parser("poweroff-check"); off.add_argument("--confirm-poweroff",action="store_true")
    ota=sub.add_parser("ota"); ota.add_argument("--confirm"); ota.add_argument("--manifest",default=str(DEFAULT_MANIFEST)); ota.add_argument("--ota-file",default=str(DEFAULT_OTA))
    return p

def main():
    args=parser().parse_args()
    if args.command=="selftest": selftest(); return 0
    log=EventLog(args.command,Path(args.log_dir)); log.emit("start",command=args.command,address=args.address,name=args.name)
    commands={"discover":cmd_discover,"monitor":cmd_monitor,"poweroff-check":cmd_poweroff,"ota":cmd_ota}
    try: asyncio.run(commands[args.command](args,log)); log.emit("pass",command=args.command); return 0
    except KeyboardInterrupt: log.emit("cancelled"); return 130
    except Exception as exc: log.emit("fail",error=str(exc)); print(f"ERROR: {exc}",file=sys.stderr); return 1
if __name__=="__main__": raise SystemExit(main())
