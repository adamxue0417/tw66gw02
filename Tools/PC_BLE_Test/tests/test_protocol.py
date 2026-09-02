import hashlib,json,sys,tempfile,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from protocol import *

class ProtocolTests(unittest.TestCase):
    def test_crc_vectors(self):
        self.assertEqual(crc16_ccitt(b"123456789"),0x29B1); self.assertEqual(crc32_iso_hdlc(b"123456789"),0xCBF43926)
    def test_frame_round_trip(self):
        frame=decode_frame(encode_frame(TYPE_CMD,0x42,b"\xff\x01")); self.assertEqual((frame.sequence,frame.payload),(0x42,b"\xff\x01"))
    def test_split_coalesced_bad_crc_resync(self):
        first=encode_frame(TYPE_CMD,1,b"\x01\x01"); second=encode_frame(TYPE_CMD,2,b"\xff\x00"); bad=bytearray(first); bad[-1]^=1
        parser=FrameParser(); frames=parser.feed(b"noise"+bytes(bad)+first[:5]); frames+=parser.feed(first[5:]+second)
        self.assertEqual([f.sequence for f in frames],[1,2])
    def test_manifest_rejects_mutation(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp); app=b"A"*200; artifact=app+b"S"*384; ota=root/"x.ota"; mf=root/"x.json"; ota.write_bytes(artifact)
            manifest={"target_version":104,"security_version":104,"application_size":len(app),"signature_size":384,"artifact_size":len(artifact),
              "crc32_iso_hdlc":f"0x{crc32_iso_hdlc(artifact):08X}","application_sha256":hashlib.sha256(app).hexdigest(),"artifact_sha256":hashlib.sha256(artifact).hexdigest(),
              "signature_algorithm":"RSA-3072-PKCS1-v1_5-SHA256","signing_key_class":"DEV","development_signature_placeholder":False}
            mf.write_text(json.dumps(manifest),encoding="utf-8"); load_and_validate_artifact(mf,ota); ota.write_bytes(artifact[:-1]+b"X")
            with self.assertRaises(ValueError): load_and_validate_artifact(mf,ota)
if __name__=="__main__": unittest.main()
