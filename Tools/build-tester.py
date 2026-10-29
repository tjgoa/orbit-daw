#!/usr/bin/env python3
"""Reserve a numbered Release build and create a self-contained tester download.
--reserved is for the native CI matrix sharing an already reserved invocation.
"""
from pathlib import Path
import argparse, datetime, hashlib, importlib.util, json, platform, plistlib, re, shutil, subprocess, sys, tarfile, zipfile
ROOT=Path(__file__).resolve().parents[1]
COUNTER=ROOT/'DEV_BUILD_COUNTER.json'
def run(args,**kw):subprocess.run([str(x) for x in args],check=True,**kw)
def reserve():
    data=json.loads(COUNTER.read_text());build=max(data['product_checkpoint_build'],data.get('last_reserved_product_build',0))+1
    cmake=ROOT/'CMakeLists.txt';s=cmake.read_text();old=re.search(r'project\(OrbitDAW VERSION ([\d.]+)',s).group(1)
    v=list(map(int,old.split('.')));v[2]+=1;version='.'.join(map(str,v))
    s,n=re.subn(r'project\(OrbitDAW VERSION [\d.]+',f'project(OrbitDAW VERSION {version}',s,count=1);assert n==1
    for pattern,replacement in [(r'BUILD_VERSION "\d+"',f'BUILD_VERSION "{build}"'),(r'MACOSX_BUNDLE_BUNDLE_VERSION "\d+"',f'MACOSX_BUNDLE_BUNDLE_VERSION "{build}"')]:
        s,n=re.subn(pattern,replacement,s,count=1);assert n==1
    cmake.write_text(s)
    p=ROOT/'Source/Main.cpp';s=p.read_text()
    s,n=re.subn(r'buildLabel\.setText\("Build \d+  -  v[\d.]+"',f'buildLabel.setText("Build {build}  -  v{version}"',s,count=1);assert n==1
    s,n=re.subn(r'getApplicationVersion\(\) override \{ return "[\d.]+"; \}',f'getApplicationVersion() override {{ return "{version}"; }}',s,count=1);assert n==1;p.write_text(s)
    data['last_reserved_product_build']=build;COUNTER.write_text(json.dumps(data,indent=2)+'\n')
    with (ROOT/'BUILD_HISTORY.md').open('a') as f:f.write(f'| {build} | {version} | {datetime.date.today().isoformat()} | Numbered tester Release: MIDI takes, portable native build setup, Event Editor correction logged. | Reserved; build in progress. |\n')
    return build,version

def verify():
    data=json.loads(COUNTER.read_text());s=(ROOT/'CMakeLists.txt').read_text()
    build=int(re.search(r'BUILD_VERSION "(\d+)"',s).group(1));version=re.search(r'project\(OrbitDAW VERSION ([\d.]+)',s).group(1)
    assert build==data['last_reserved_product_build'],'Reserve the next unused build before compiling.'
    assert f'Build {build}  -  v{version}' in (ROOT/'Source/Main.cpp').read_text(),'Displayed build/version mismatch'
    return build,version

def source_zip(path):
    spec=importlib.util.spec_from_file_location('source_package',ROOT/'Tools/package-source-archive.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    with zipfile.ZipFile(path,'x',zipfile.ZIP_DEFLATED,compresslevel=8) as z:
        for p in module.eligible_files():z.write(p,p.relative_to(ROOT).as_posix())
        z.writestr('SOURCE_CHECKPOINT.txt','Complete Orbit-owned source/configuration and sampler overlay. Fetch the pinned upstream dependencies and run Tools/prepare-dependencies.py. See DEPENDENCIES.md and DISTRIBUTION.md. No compiled application or unchanged vendor dependencies are included.\n')
    with zipfile.ZipFile(path) as z:assert z.testzip() is None

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform',choices=['macos-universal','windows-x64','linux-x64'])
    parser.add_argument('--reserved',action='store_true');parser.add_argument('--verify-reservation',action='store_true')
    parser.add_argument('--jobs',type=int,default=2);parser.add_argument('--install-current',action='store_true')
    args=parser.parse_args()
    if args.verify_reservation:print('Reserved build:',verify());return
    if not args.platform:parser.error('--platform is required')
    expected={'macos-universal':'Darwin','windows-x64':'Windows','linux-x64':'Linux'}[args.platform]
    if platform.system()!=expected:raise SystemExit(f'{args.platform} must be built on a native {expected} runner.')
    build,version=verify() if args.reserved else reserve()
    dist=ROOT/'Builds/Testers';dist.mkdir(parents=True,exist_ok=True)
    attempt=dist/f'.attempt-{build}-{args.platform}.json'
    if attempt.exists():raise SystemExit('This platform attempt already consumed its number. Start a fresh numbered invocation.')
    attempt.write_text(json.dumps(dict(build=build,version=version,platform=args.platform,status='started'),indent=2)+'\n')
    folder=ROOT/'build'/f'testers-{args.platform}'
    configure=['cmake','-S',ROOT,'-B',folder,'-DCMAKE_BUILD_TYPE=Release']
    if expected=='Darwin':configure+=['-G','Unix Makefiles','-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64','-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0']
    elif expected=='Windows':configure+=['-G','Visual Studio 17 2022','-A','x64','-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded']
    else:configure+=['-G','Unix Makefiles']
    stage=dist/f'Orbit-Build-{build}-{args.platform}'
    if stage.exists():raise SystemExit(f'Refusing to overwrite {stage}')
    try:
        run(configure);run(['cmake','--build',folder,'--config','Release','--target','Orbit','--parallel',str(args.jobs)])
        output=folder/'Orbit_artefacts/Release';stage.mkdir()
        if expected=='Darwin':
            app=output/'Orbit.app';info=plistlib.loads((app/'Contents/Info.plist').read_bytes())
            assert str(info['CFBundleVersion'])==str(build) and info['CFBundleShortVersionString']==version
            arches=subprocess.check_output(['lipo','-archs',app/'Contents/MacOS/Orbit'],text=True).split()
            assert {'arm64','x86_64'}.issubset(arches),f'Not universal: {arches}'
            shutil.copytree(app,stage/'Orbit.app',symlinks=True)
            run(['codesign','--force','--sign','-',stage/'Orbit.app']);run(['codesign','--verify','--deep','--strict',stage/'Orbit.app'])
        elif expected=='Windows':shutil.copy2(output/'Orbit.exe',stage/'Orbit.exe')
        else:
            shutil.copy2(output/'Orbit',stage/'Orbit')
            dependencies=subprocess.check_output(['ldd',stage/'Orbit'],text=True);assert 'not found' not in dependencies
            (stage/'RUNTIME_LIBRARIES.txt').write_text(dependencies)
        if not args.reserved:
            data=json.loads(COUNTER.read_text());data.update(product_checkpoint_build=build,product_checkpoint_version=version,successful_dev_iterations=data.get('successful_dev_iterations',0)+1,last_successful_build_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='seconds'))
            COUNTER.write_text(json.dumps(data,indent=2)+'\n')
            p=ROOT/'README.md';s=p.read_text();s=re.sub(r'The current source checkpoint is Build \d+ / v[\d.]+',f'The current source checkpoint is Build {build} / v{version}',s,count=1);s=re.sub(r'local development iteration \*\*\d+\*\*',f"local development iteration **{data['successful_dev_iterations']}**",s,count=1);p.write_text(s)
            p=ROOT/'BUILD_HISTORY.md';p.write_text(p.read_text().replace(f'| Reserved; build in progress. |',f'| Release built: {args.platform}; native MIDI take regression passed; tester package created. |'))
        shutil.copy2(ROOT/'DISTRIBUTION.md',stage/'READ_ME_FIRST.md');shutil.copy2(ROOT/'LICENSE',stage/'LICENSE')
        licenses=stage/'Third-party notices'
        for p in (ROOT/'Dependencies/tracktion_engine').rglob('*'):
            if p.is_file() and '.git' not in p.parts and re.match(r'^(licen[cs]e|copying|notice)([._-]|$)',p.name,re.I):
                dest=licenses/p.relative_to(ROOT/'Dependencies');dest.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,dest)
        source_zip(stage/f'Orbit-Build-{build}-Source.zip')
        (stage/'BUILD.json').write_text(json.dumps(dict(build=build,version=version,platform=args.platform,configuration='Release',signing='ad-hoc, not notarized' if expected=='Darwin' else 'unsigned',source_included=True),indent=2)+'\n')
        if expected=='Darwin':
            archive=dist/f'{stage.name}.zip';run(['ditto','-c','-k','--sequesterRsrc','--keepParent',stage,archive])
        elif expected=='Windows':
            archive=dist/f'{stage.name}.zip'
            with zipfile.ZipFile(archive,'x',zipfile.ZIP_DEFLATED,compresslevel=8) as z:
                for p in stage.rglob('*'):
                    if p.is_file():z.write(p,p.relative_to(dist))
        else:
            archive=dist/f'{stage.name}.tar.gz'
            with tarfile.open(archive,'x:gz') as t:t.add(stage,arcname=stage.name)
        if archive.suffix=='.zip':
            with zipfile.ZipFile(archive) as z:
                assert z.testzip() is None
                assert not any(Path(n).is_absolute() or '..' in Path(n).parts for n in z.namelist())
        else:
            with tarfile.open(archive) as t:assert not any(Path(n).is_absolute() or '..' in Path(n).parts for n in t.getnames())
        digest=hashlib.sha256(archive.read_bytes()).hexdigest();(dist/(archive.name+'.sha256')).write_text(digest+'  '+archive.name+'\n')
        if args.install_current:
            if expected!='Darwin':raise SystemExit('--install-current is macOS only')
            current=ROOT/'Builds/Current/Orbit.app'
            if current.exists():
                old=plistlib.loads((current/'Contents/Info.plist').read_bytes())['CFBundleVersion'];backup=ROOT/f'Builds/Previous/Orbit_Build_{old}_before_{build}.app'
                if backup.exists():raise SystemExit(f'Backup exists; refusing to replace it: {backup}')
                backup.parent.mkdir(parents=True,exist_ok=True);shutil.move(current,backup)
            shutil.copytree(stage/'Orbit.app',current,symlinks=True)
        attempt.write_text(json.dumps(dict(build=build,version=version,platform=args.platform,status='succeeded',archive=archive.name,sha256=digest,size=archive.stat().st_size),indent=2)+'\n')
        print(f'TESTER DOWNLOAD: {archive}\nSIZE: {archive.stat().st_size:,} bytes\nSHA256: {digest}',flush=True)
    except BaseException:
        attempt.write_text(json.dumps(dict(build=build,version=version,platform=args.platform,status='failed-number-consumed'),indent=2)+'\n')
        if not args.reserved:
            p=ROOT/'BUILD_HISTORY.md';s=p.read_text();s=s.replace('| Reserved; build in progress. |','| Tester build failed; build number consumed. |');p.write_text(s)
        raise
if __name__=='__main__':main()
