#!/usr/bin/env python3
"""Stage a reserved universal Release locally; retain prior app and installation notes."""
from pathlib import Path
import datetime,hashlib,json,os,plistlib,re,shutil,subprocess,sys
root=Path(__file__).resolve().parents[1]
counter_path=root/'DEV_BUILD_COUNTER.json';counter=json.loads(counter_path.read_text());build=counter['last_reserved_product_build'];previous=counter['product_checkpoint_build']
assert build>previous
cmake=(root/'CMakeLists.txt').read_text();version=re.search(r'project\(OrbitDAW VERSION ([0-9.]+)',cmake)[1]
assert f'BUILD_VERSION "{build}"' in cmake
built=root/'build/testers-macos-universal/Orbit_artefacts/Release/Orbit.app';info=plistlib.loads((built/'Contents/Info.plist').read_bytes())
assert str(info['CFBundleVersion'])==str(build) and info['CFBundleShortVersionString']==version
for notice in ['License.txt','OOURA-LICENSE.txt','UPSTREAM.md']:assert (built/'Contents/Resources/Third-party notices/Open303'/notice).is_file()
for notice in ['LICENSE','QUAMPLEX-LICENSE','UPSTREAM.md']:assert (built/'Contents/Resources/Third-party notices/Geonkick'/notice).is_file()
for notice in ['LICENSE','NOTICE','UPSTREAM.md']:assert (built/'Contents/Resources/Third-party notices/9W9'/notice).is_file()
for artwork in ['silver-flow.png','ambient-glow.png','champagne-wave.png','reference-ribbon.png','compact-silver.png']:
    source=root/'Source/Options/Artwork'/artwork;packaged=built/'Contents/Resources/Header Artwork'/artwork
    assert packaged.is_file() and hashlib.sha256(source.read_bytes()).digest()==hashlib.sha256(packaged.read_bytes()).digest(),f'Missing or stale header artwork: {artwork}'
arches=subprocess.check_output(['lipo','-archs',built/'Contents/MacOS/Orbit'],text=True).split();assert set(arches)=={'arm64','x86_64'}
assert f'Build {build}  -  v{version}'.encode() in (built/'Contents/MacOS/Orbit').read_bytes()
subprocess.run(['codesign','--force','--sign','-',str(built)],check=True);subprocess.run(['codesign','--verify','--deep','--strict',str(built)],check=True)
current=root/'Builds/Current/Orbit.app';old=plistlib.loads((current/'Contents/Info.plist').read_bytes());assert str(old['CFBundleVersion'])==str(previous)
backup=root/f'Builds/Previous/Orbit_Build_{previous}_before_{build}.app';staged=root/f'Builds/Current/Orbit.install-{build}.app';assert not backup.exists() and not staged.exists()
notes=root/'Builds/Current/Build Notes';notes.mkdir(exist_ok=True)
receipt=notes/'BUILD_INSTALLATION.json';receipt_backup=receipt.with_name(f'BUILD_INSTALLATION.before{build}.json');assert receipt.exists() and not receipt_backup.exists()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();previous_sha=sha(current/'Contents/MacOS/Orbit')
shutil.copytree(built,staged,symlinks=True);subprocess.run(['codesign','--verify','--deep','--strict',str(staged)],check=True)
current.rename(backup);staged.rename(current);os.utime(current,None);shutil.copy2(receipt,receipt_backup)
counter.update(product_checkpoint_build=build,product_checkpoint_version=version,successful_dev_iterations=counter['successful_dev_iterations']+1,last_successful_build_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(timespec='seconds'));counter_path.write_text(json.dumps(counter,indent=2)+'\n')
receipt.write_text(json.dumps(dict(build=build,version=version,configuration='Release',architectures=arches,executable_sha256=sha(current/'Contents/MacOS/Orbit'),previous_build=previous,previous_bundle_backup=str(backup),previous_executable_sha256=previous_sha,running_app_was_not_closed=True),indent=2)+'\n')
p=root/'README.md';s=p.read_text();s=re.sub(r'local development iteration \*\*\d+\*\*',f"local development iteration **{counter['successful_dev_iterations']}**",s,count=1);p.write_text(s)
p=root/'BUILD_HISTORY.md';lines=p.read_text().splitlines();matches=0
for i,line in enumerate(lines):
 if line.startswith(f'| {build} |'):
  parts=line.split('|');parts[-2]=' Local universal Release built, signed/verified and installed. '+sys.argv[1]+' ';lines[i]='|'.join(parts);matches+=1
assert matches==1;p.write_text('\n'.join(lines)+'\n')
p=root/'PROGRESS.md';s=p.read_text().replace(f'Build {build} universal Release reserved; installation pending.',f'Build {build} / v{version} compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build {previous} is preserved at Builds/Previous/Orbit_Build_{previous}_before_{build}.app. The running app was not closed. Successful development iteration {counter["successful_dev_iterations"]}.',1);p.write_text(s)
print(f'Installed Build {build} / v{version}, iteration {counter["successful_dev_iterations"]}. Previous app preserved: {backup}')
