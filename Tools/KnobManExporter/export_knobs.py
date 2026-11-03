#!/usr/bin/env python3
"""Local, pinned WebKnobMan sprite exporter. Source projects are read-only."""
from __future__ import annotations
import argparse, base64, contextlib, hashlib, json, os, re, shutil, subprocess, sys
import tempfile, threading, time, webbrowser
from datetime import datetime, timezone
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

VERSION = '0.2.0'
RENDERER_COMMIT = 'c3791b631fb663f899850b1dbff9b2426296851e'
RENDERER_URL = 'https://github.com/g200kg/webknobman.git'
HERE = Path(__file__).resolve().parent


def sha(data):
    return hashlib.sha256(data).hexdigest()


def atomic_json(path, data):
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', dir=path.parent, delete=False) as f:
        json.dump(data, f, indent=2, ensure_ascii=False); f.write('\n'); tmp = Path(f.name)
    try:
        os.replace(tmp, path)
    finally:
        tmp.unlink(missing_ok=True)


def profile(data):
    """Read export preferences without running or modifying the editable file."""
    if len(data) > 32 * 1024 * 1024:
        raise ValueError('Source exceeds the 32 MiB input limit')
    if data[:2] == b'KM':
        offset = int.from_bytes(data[2:6], 'little')
        if not 6 <= offset < len(data):
            raise ValueError('Invalid KM profile offset')
        payload = data[offset:]
        # KM exists in both legacy UTF-16LE and newer byte-text variants.
        # Some original writers leave one unmatched final NUL/newline byte.
        if payload.startswith((b'[\x00', b'\xff\xfe')):
            text = payload.decode('utf-16-le', errors='replace').lstrip('\ufeff').rstrip('\x00\ufffd')
        else:
            try: text = payload.decode('utf-8-sig')
            except UnicodeDecodeError: text = payload.decode('latin-1')
    elif data[:2] in (b'\xff\xfe', b'\xfe\xff'):
        text = data.decode('utf-16')
    elif data.startswith(b'\x89PNG'):
        offset = 8; text = ''
        while offset + 12 <= len(data):
            size = int.from_bytes(data[offset:offset+4], 'big')
            if offset + 12 + size > len(data):
                raise ValueError('Truncated PNG project')
            if data[offset+4:offset+8] == b'tEXt':
                # Upstream's legacy PNG profile uses an eight-byte text keyword.
                text = data[offset+16:offset+8+size].decode('latin-1'); break
            offset += size + 12
        if not text:
            raise ValueError('PNG has no supported KnobMan text profile')
    else:
        text = data.decode('utf-8-sig')
    sections = {}; current = None
    for line in text.splitlines():
        if line.startswith('[') and line.endswith(']'):
            current = sections.setdefault(line[1:-1], {})
        elif current is not None and '=' in line:
            key, value = line.split('=', 1); current[key] = value
    prefs = sections.get('Prefs')
    if not prefs:
        raise ValueError('Missing [Prefs] section')
    def integer(key, default):
        value = float(prefs.get(key, default))
        if not value.is_integer():
            raise ValueError(f'Non-integer {key}')
        return int(value)
    count = integer('NumOfImage', 0) or integer('RenderFrames', 31)
    result = dict(frame_width=integer('OutputSizeX', 32), frame_height=integer('OutputSizeY', 32),
                  frames=count, orientation='horizontal' if integer('AlignHorizontal', 0) else 'vertical',
                  layers=integer('Layers', 1))
    if not (1 <= result['layers'] <= 60 and 1 <= count <= 2048):
        raise ValueError('Unsupported layer/frame count (1–60 layers, 1–2048 frames)')
    for i in range(1, result['layers'] + 1):
        if f'Layer{i}' not in sections:
            raise ValueError(f'Missing Layer{i}; project may be truncated')
    result['credits'] = {f'{section}.{key}': value for section, values in sections.items()
                         for key, value in values.items()
                         if key.lower() in {'author','copyright','license','licence','comment','url','designer'} and value}
    return result


def dimensions(meta, max_pixels=32_000_000):
    w, h, n = (int(meta[k]) for k in ('frame_width', 'frame_height', 'frames'))
    if min(w,h,n) < 1 or max(w,h) > 4096 or w*h*n > max_pixels:
        raise ValueError('Invalid or oversized sheet; reduce size/frames or raise --max-pixels')
    # Always render vertically upstream, then rearrange complete frames losslessly.
    if h*n > 32767 or (meta['orientation'] == 'horizontal' and w*n > 32767):
        raise ValueError('Sheet exceeds Chromium’s supported 32767-pixel dimension; reduce size/frames')
    return (w*n,h) if meta['orientation'] == 'horizontal' else (w,h*n)


def validate_png(path, meta, allow_blank=False):
    from PIL import Image
    want = dimensions(meta, max_pixels=1_000_000_000)
    with Image.open(path) as original:
        if original.format != 'PNG' or original.size != want or getattr(original, 'n_frames', 1) != 1:
            raise ValueError(f'Expected static PNG sheet {want}, got {original.format} {original.size}')
        original.load(); img = original.convert('RGBA')
    if not img.getchannel('A').getbbox():
        raise ValueError('Entire sprite is transparent; output rejected')
    w,h,n = (meta[k] for k in ('frame_width','frame_height','frames'))
    blank=[]; hashes=[]; bounds=[]
    for f in range(n):
        x,y = (f*w,0) if meta['orientation']=='horizontal' else (0,f*h)
        frame=img.crop((x,y,x+w,y+h)); bbox=frame.getchannel('A').getbbox()
        if bbox is None: blank.append(f)
        bounds.append(bbox); hashes.append(sha(frame.tobytes()))
    if blank and not allow_blank:
        raise ValueError(f'Transparent frames {blank[:20]}; review source or explicitly use --allow-blank-frames')
    warnings=[]
    if blank: warnings.append(f'{len(blank)} transparent frames explicitly allowed')
    if len(set(hashes))==1 and n>1: warnings.append('Every frame is identical; this may be an unanimated design')
    return dict(png_sha256=sha(path.read_bytes()), rgba_sha256=sha(img.tobytes()),
                frame_sha256=hashes, alpha_bounds=bounds, blank_frames=blank, warnings=warnings,
                bytes=path.stat().st_size)


def ensure_renderer(target):
    if not target.exists():
        target.parent.mkdir(parents=True, exist_ok=True)
        temp=Path(tempfile.mkdtemp(prefix='renderer-',dir=target.parent))
        try:
            subprocess.run(['git','clone','--no-checkout',RENDERER_URL,str(temp)], check=True)
            subprocess.run(['git','-C',str(temp),'checkout','--detach',RENDERER_COMMIT], check=True)
            temp.rename(target)
        finally:
            if temp.exists(): shutil.rmtree(temp)
    head=subprocess.check_output(['git','-C',str(target),'rev-parse','HEAD'],text=True).strip()
    dirty=subprocess.check_output(['git','-C',str(target),'status','--porcelain','--untracked-files=no'],text=True).strip()
    if head != RENDERER_COMMIT or dirty:
        raise ValueError('Renderer must be an unmodified checkout of pinned commit '+RENDERER_COMMIT)
    if not (target/'index.html').is_file() or not (target/'LICENSE').is_file():
        raise ValueError('Incomplete WebKnobMan checkout')


class LocalHandler(SimpleHTTPRequestHandler):
    def log_message(self, *_): pass
    def end_headers(self):
        self.send_header('Cache-Control','no-store'); super().end_headers()
    def do_GET(self):
        if any(part.startswith('.') for part in urlsplit(self.path).path.split('/') if part):
            self.send_error(404); return
        super().do_GET()


@contextlib.contextmanager
def serve(folder):
    server=ThreadingHTTPServer(('127.0.0.1',0),partial(LocalHandler,directory=str(folder)))
    thread=threading.Thread(target=server.serve_forever,daemon=True); thread.start()
    try: yield f'http://127.0.0.1:{server.server_port}'
    finally: server.shutdown(); server.server_close(); thread.join()


LOAD_JS = r'''async ({name, data}) => {
  window.orbitLoaded=false; window.orbitDialogs=[];
  const popup=PopupMsg;
  PopupMsg=function(s){popup(s);if(s==='')window.orbitLoaded=true;};
  const timeout=window.setTimeout.bind(window);
  window.setTimeout=(fn,delay,...args)=>timeout(fn,fn===Render?0:delay,...args);
  const binary=atob(data), bytes=Uint8Array.from(binary,c=>c.charCodeAt(0));
  OpenFile(new File([bytes],name,{type:'application/octet-stream'}));
}'''
READY_JS = r'''async () => {
  const images=[...loadingimg,...loadingtex].filter(i=>i&&i.src);
  await Promise.all(images.map(i=>i.decode()));
  await document.fonts.ready;
  const problems=[]; const fonts=[];
  for(const [i,l] of layers.entries()) {
    if(!l.visible)continue;
    if(l.prim.type.val===1 && !(l.prim.image&&l.prim.image.naturalWidth))problems.push('Layer '+(i+1)+': missing image');
    if(l.prim.texturedepth.val>0 && l.prim.texturename && l.prim.texturename!=='None' && !(l.prim.tex.img&&l.prim.tex.img.naturalWidth))problems.push('Layer '+(i+1)+': missing texture');
    if(l.prim.type.val===14)fonts.push(l.prim.fontname);
  }
  if(problems.length)throw Error(problems.join('; '));
  return {frame_width:prefs.width,frame_height:prefs.height,frames:prefs.framesrender.val,layers:layers.length,fonts};
}'''
EXPORT_JS = r'''({width,height,frames}) => {setTimeout(()=>{
  // Use original Export and Render routines. Only timer pacing changes.
  prefs.paramwidth.val=prefs.width=width; prefs.paramheight.val=prefs.height=height;
  prefs.framesrender.val=frames; prefs.framesrender.Disp();
  document.getElementById('exportas').selectedIndex=0;
  document.getElementById('orientation').selectedIndex=0;
  for(const l of layers)l.SetSize(1);
  Export();
},0);}'''


def render(context, url, source, data, meta, tmp, timeout):
    page=context.new_page(); page.set_default_timeout(timeout*1000)
    errors=[]; dialogs=[]
    page.on('pageerror',lambda e:errors.append(str(e)))
    def dialog(d):
        dialogs.append(d.message); d.dismiss()
    page.on('dialog',dialog)
    try:
        page.goto(url+'/index.html',wait_until='load')
        page.wait_for_function("typeof OpenFile==='function' && typeof prefs!=='undefined' && prefs && layers.length>0")
        page.evaluate(LOAD_JS,dict(name=source.name,data=base64.b64encode(data).decode()))
        page.wait_for_function('window.orbitLoaded===true')
        loaded=page.evaluate(READY_JS)
        original=profile(data)
        if any(loaded[k]!=original[k] for k in ('frame_width','frame_height','frames','layers')):
            raise ValueError(f'Loaded metadata mismatch: {loaded} versus {original}')
        if errors or dialogs: raise ValueError('; '.join(errors+dialogs))
        with page.expect_download(timeout=timeout*1000) as event:
            page.evaluate(EXPORT_JS,dict(width=meta['frame_width'],height=meta['frame_height'],frames=meta['frames']))
        event.value.save_as(tmp)
        if errors or dialogs: raise ValueError('; '.join(errors+dialogs))
        if meta['orientation']=='horizontal':
            from PIL import Image
            w,h,n=(meta[k] for k in ('frame_width','frame_height','frames'))
            with Image.open(tmp) as v:
                v.load()
                if v.size!=(w,h*n): raise ValueError('Incomplete vertical render before layout conversion')
                out=Image.new('RGBA',(w*n,h))
                for f in range(n): out.paste(v.crop((0,f*h,w,(f+1)*h)),(f*w,0))
            out.save(tmp,format='PNG')
        return loaded['fonts']
    finally:
        page.close()


def gallery(output, records):
    # Embed exact PNG bytes; file:// works and requires no server or network.
    gallery_data=[]
    for rec in records:
        view={k:rec[k] for k in ('source','status','error','frame_width','frame_height','frames','orientation','warnings','sprite') if k in rec}
        if rec.get('status')=='ok':
            view['image']='data:image/png;base64,'+base64.b64encode((output/rec['sprite']).read_bytes()).decode()
        gallery_data.append(view)
    template=(HERE/'gallery-template.html').read_text()
    payload=json.dumps(gallery_data,ensure_ascii=True).replace('<','\\u003c')
    tmp=output/'index.html.tmp';tmp.write_text(template.replace('/*ORBIT_DATA*/[]',payload),encoding='utf-8');os.replace(tmp,output/'index.html')


def pick_folders():
    if sys.platform!='darwin': raise ValueError('--choose uses macOS folder dialogs; use CLI paths elsewhere')
    def choose(prompt):
        script='POSIX path of (choose folder with prompt '+json.dumps(prompt)+')'
        result=subprocess.run(['osascript','-e',script],capture_output=True,text=True)
        if result.returncode: raise KeyboardInterrupt
        return Path(result.stdout.strip())
    return choose('Choose the folder containing your .knob projects'),choose('Choose a separate folder for the exported sprites')


def main(argv=None):
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('sources',nargs='*',type=Path,help='Individual .knob files and/or folders, recursively')
    p.add_argument('-o','--output',type=Path)
    p.add_argument('--choose',action='store_true',help='Choose source/output folders using macOS dialogs')
    p.add_argument('--renderer',type=Path,default=HERE/'.runtime'/'webknobman')
    p.add_argument('--browser',choices=['chrome','chromium'],default='chrome')
    p.add_argument('--orientation',choices=['source','vertical','horizontal'],default='source')
    p.add_argument('--size',type=int,help='Square frame size override; source unchanged')
    p.add_argument('--frames',type=int,help='Frame count override; source unchanged')
    p.add_argument('--timeout',type=int,default=240)
    p.add_argument('--limit',type=int,default=0)
    p.add_argument('--max-pixels',type=int,default=32_000_000)
    p.add_argument('--retry',action='store_true',help='Render again rather than reuse validated outputs')
    p.add_argument('--allow-blank-frames',action='store_true',help='Allow intentionally transparent states, with a warning')
    p.add_argument('--open',action='store_true',help='Open the local scrub gallery when done')
    a=p.parse_args(argv)
    if a.choose:
        source,a.output=pick_folders();a.sources=[source]
    if not a.sources or a.output is None: p.error('Supply source(s) and --output, or use --choose')
    if a.timeout<1 or a.limit<0: p.error('Timeout must be positive; limit cannot be negative')
    output=a.output.expanduser().resolve(); files={}
    for raw in a.sources:
        src=raw.expanduser().resolve()
        if not src.exists(): p.error(f'Source does not exist: {src}')
        found=[src] if src.is_file() else sorted(src.rglob('*'))
        for path in found:
            if path.is_file() and path.suffix.lower()=='.knob': files[str(path.resolve())]=path.resolve()
    paths=sorted(files.values(),key=lambda x:str(x).casefold())
    if a.limit: paths=paths[:a.limit]
    if not paths: p.error('No .knob projects found')
    if output in [src.expanduser().resolve() for src in a.sources]: p.error('Choose a separate output folder')
    output.mkdir(parents=True,exist_ok=True);(output/'sprites').mkdir(exist_ok=True)
    # A kernel lock is released automatically on crash; no stale lock cleanup.
    import fcntl
    lock=(output/'.export.lock').open('a')
    try: fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
    except BlockingIOError: p.error('Another export is already using this output folder')
    old={}
    if (output/'manifest.json').exists():
        previous=json.loads((output/'manifest.json').read_text())
        if previous.get('format')!='orbit-sprite-manifest-v1': raise ValueError('Unknown existing manifest format; choose a fresh output')
        old={r['id']:r for r in previous['knobs']}
    ensure_renderer(a.renderer.expanduser().resolve())
    from playwright.sync_api import sync_playwright
    successes=0;failed=0
    def save():
        atomic_json(output/'manifest.json',dict(format='orbit-sprite-manifest-v1',exporter_version=VERSION,
                    renderer=dict(url=RENDERER_URL,commit=RENDERER_COMMIT),updated_utc=datetime.now(timezone.utc).isoformat(),knobs=list(old.values())))
    with serve(a.renderer.expanduser().resolve()) as url, sync_playwright() as pw:
        browser=pw.chromium.launch(headless=True,**({'channel':'chrome'} if a.browser=='chrome' else {}))
        context=browser.new_context(accept_downloads=True)
        # Renderer runs offline: block ads/fonts/uploads and any non-loopback URL.
        context.route('**/*',lambda route:route.continue_() if route.request.url.startswith((url+'/', 'data:', 'blob:')) else route.abort())
        try:
            for index,source in enumerate(paths,1):
                identity=sha(str(source).encode())[:20]
                rec=dict(id=identity,source=str(source),status='error')
                print(f'[{index}/{len(paths)}] {source.name}',flush=True)
                temp=None
                try:
                    data=source.read_bytes(); source_hash=sha(data);meta=profile(data)
                    if a.size is not None: meta.update(frame_width=a.size,frame_height=a.size)
                    if a.frames is not None: meta['frames']=a.frames
                    if a.orientation!='source': meta['orientation']=a.orientation
                    dimensions(meta,a.max_pixels)
                    rec.update(meta,source_sha256=source_hash,renderer_commit=RENDERER_COMMIT,browser_version=browser.version)
                    key=sha(json.dumps([source_hash,meta,RENDERER_COMMIT],sort_keys=True).encode())
                    rec['render_key']=key
                    reusable=None
                    if not a.retry:
                        for candidate in old.values():
                            if candidate.get('status')!='ok' or candidate.get('render_key')!=key: continue
                            dest=(output/candidate['sprite']).resolve()
                            if output not in dest.parents: continue
                            try:
                                validation=validate_png(dest,meta,a.allow_blank_frames)
                                if validation['png_sha256']!=candidate.get('png_sha256'): continue
                                reusable=(candidate,validation);break
                            except (ValueError,OSError): pass
                    if reusable:
                        candidate,validation=reusable;rec.update(validation,sprite=candidate['sprite'],status='ok',reused=True,
                                   browser_version=candidate.get('browser_version','unknown'),fonts=candidate.get('fonts',[]))
                        if candidate['id']!=identity:rec['duplicate_of']=candidate['id']
                        print('  Reused validated sheet',flush=True)
                    else:
                        with tempfile.NamedTemporaryFile(dir=output,suffix='.png',delete=False) as f:temp=Path(f.name)
                        fonts=render(context,url,source,data,meta,temp,a.timeout)
                        validation=validate_png(temp,meta,a.allow_blank_frames)
                        slug=re.sub(r'[^a-zA-Z0-9_-]+','_',source.stem)[:64] or 'knob'
                        # Content/settings-based suffix prevents collisions and preserves older variants.
                        relative=f'sprites/{slug}-{key[:16]}.png';dest=output/relative
                        os.replace(temp,dest);temp=None
                        rec.update(validation,sprite=relative,status='ok',fonts=fonts,reused=False)
                        print(f'  OK: {meta["frames"]} × {meta["frame_width"]}×{meta["frame_height"]} ({meta["orientation"]})',flush=True)
                    if sha(source.read_bytes())!=source_hash: raise ValueError('Source changed during export; retry when editing has stopped')
                    atomic_json(output/rec['sprite'].replace('.png','.json'),{**rec,'source':str(source)})
                    successes+=1
                except Exception as error:
                    rec.update(status='error',error=str(error));failed+=1;print('  FAILED: '+str(error),flush=True)
                finally:
                    if temp is not None:temp.unlink(missing_ok=True)
                    old[identity]=rec;save()
            gallery(output,list(old.values()))
        finally:
            try: gallery(output,list(old.values()))
            finally: context.close();browser.close()
    print(f'Finished: {successes} OK, {failed} failed. Gallery: {output/"index.html"}',flush=True)
    if a.open: webbrowser.open((output/'index.html').as_uri())
    return 1 if failed else 0


if __name__=='__main__':
    try: sys.exit(main())
    except KeyboardInterrupt: print('Cancelled. Original projects and completed exports are retained.');sys.exit(130)
    except Exception as e: print('Exporter: '+str(e),file=sys.stderr);sys.exit(1)
