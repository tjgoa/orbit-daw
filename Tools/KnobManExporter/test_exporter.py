#!/usr/bin/env python3
"""Meaningful output/profile guards; optional real-browser integration."""
import argparse, importlib.util, json, tempfile, unittest
from pathlib import Path
from PIL import Image
import export_knobs as exporter


def fixture(colour=180):
    return f'''[Prefs]
Version=1490
Layers=2
OutputSizeX=24
OutputSizeY=24
RenderFrames=5
PreviewFrames=2
AlignHorizontal=0
[Layer1]
Primitive=CircleFill
ColR={colour}
ColG=100
ColB=80
[Layer2]
Primitive=Line
ColR=255
ColG=255
ColB=255
PrimWidth=8
PrimLength=70
Angle1=-135
Angle2=135
AnimateAngle=1
'''.encode()


class Guards(unittest.TestCase):
    def test_profile_encodings(self):
        text=fixture().decode()
        for payload in [text.encode(),text.encode('utf-16-le'),text.encode('utf-16-le')+b'\x00']:
            p=exporter.profile(b'KM\x06\x00\x00\x00'+payload)
            self.assertEqual((p['frame_width'],p['frames'],p['layers']),(24,5,2))
    def test_bad_profiles_and_memory(self):
        for data in [b'garbage',fixture().replace(b'[Layer2]',b'[Broken]'),b'KM\xff\xff\xff\xffx']:
            with self.assertRaises(ValueError):exporter.profile(data)
        for change in [dict(frame_height=1000,frames=100),dict(frame_width=4000,frame_height=4000,frames=10),dict(frames=0)]:
            meta=exporter.profile(fixture());meta.update(change)
            with self.assertRaises(ValueError):exporter.dimensions(meta)
    def test_blank_incomplete_and_static_strips(self):
        with tempfile.TemporaryDirectory() as d:
            path=Path(d)/'strip.png';meta=exporter.profile(fixture());img=Image.new('RGBA',(24,120))
            img.save(path)
            with self.assertRaisesRegex(ValueError,'Entire sprite'):exporter.validate_png(path,meta,True)
            img.paste((200,100,80,255),(0,0,24,96));img.save(path)
            with self.assertRaisesRegex(ValueError,'Transparent frames'):exporter.validate_png(path,meta)
            self.assertEqual(exporter.validate_png(path,meta,True)['blank_frames'],[4])
            img.paste((200,100,80,255),(0,0,24,120));img.save(path)
            self.assertIn('identical',exporter.validate_png(path,meta)['warnings'][0])
            Image.new('RGBA',(24,119),'red').save(path)
            with self.assertRaisesRegex(ValueError,'Expected static PNG'):exporter.validate_png(path,meta)


def integration(renderer):
    with tempfile.TemporaryDirectory(prefix='orbit-knob-integration-') as d:
        root=Path(d).resolve();a=root/'a';b=root/'b';c=root/'c'
        for folder in [a,b,c]:folder.mkdir()
        files=[folder/'same.knob' for folder in [a,b,c]]
        for path,col in zip(files,[180,180,80]):path.write_bytes(fixture(col))
        hashes={p:exporter.sha(p.read_bytes()) for p in files};out=root/'out'
        common=['-o',str(out),'--renderer',str(renderer)]
        assert exporter.main([*[str(p) for p in files],*common])==0
        records=json.loads((out/'manifest.json').read_text())['knobs']
        assert len(records)==3 and len({r['sprite'] for r in records})==2
        assert sum('duplicate_of' in r for r in records)==1
        assert all(not r['blank_frames'] for r in records)
        # Rerun subset: retain the rest of the library and validate cached content.
        assert exporter.main([str(files[0]),*common])==0
        records=json.loads((out/'manifest.json').read_text())['knobs'];assert len(records)==3
        first=next(r for r in records if r['source']==str(files[0]));assert first['reused']
        (out/first['sprite']).write_bytes(b'bad PNG')
        assert exporter.main([str(files[0]),*common])==0
        records=json.loads((out/'manifest.json').read_text())['knobs']
        first=next(r for r in records if r['source']==str(files[0]));assert not first['reused']
        vertical=Image.open(out/first['sprite']).convert('RGBA')
        assert exporter.main([str(files[0]),*common,'--orientation','horizontal'])==0
        records=json.loads((out/'manifest.json').read_text())['knobs'];assert len(records)==3
        first=next(r for r in records if r['source']==str(files[0]));horizontal=Image.open(out/first['sprite']).convert('RGBA')
        for f in range(5):assert vertical.crop((0,24*f,24,24*(f+1))).tobytes()==horizontal.crop((24*f,0,24*(f+1),24)).tobytes()
        assert exporter.main([str(files[0]),*common,'--size','32','--frames','7'])==0
        rec=next(r for r in json.loads((out/'manifest.json').read_text())['knobs'] if r['source']==str(files[0]))
        assert (rec['frame_width'],rec['frame_height'],rec['frames'])==(32,32,7)
        bad=root/'bad.knob';bad.write_bytes(b'broken')
        assert exporter.main([str(bad),*common])==1
        records=json.loads((out/'manifest.json').read_text())['knobs'];assert len(records)==4 and sum(r['status']=='ok' for r in records)==3
        assert all(exporter.sha(p.read_bytes())==hashes[p] for p in files)
        print('PASS: real renderer, same-name collision safety, content deduplication, validated resume, corrupt-cache rerender, library preservation, per-frame horizontal equality, size/frames overrides, failure recording, original-source hashes')


if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--renderer',type=Path);args=parser.parse_args()
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(Guards))
    if not result.wasSuccessful():raise SystemExit(1)
    if args.renderer:integration(args.renderer)
