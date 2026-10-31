#!/usr/bin/env python3
"""Publish existing, pinned CI artifacts. Never rebuild or replace a download."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import io
import json
import mimetypes
import os
import tarfile
import urllib.error
import urllib.parse
import urllib.request
import zipfile

ROOT=Path(__file__).resolve().parents[1]

def sha(data):
    return hashlib.sha256(data).hexdigest()

def safe_names(names):
    assert all(not PurePosixPath(n).is_absolute() and '..' not in PurePosixPath(n).parts
               and '\\' not in n for n in names), 'Unsafe archive path'

def inspect_package(data,build,version,platform):
    prefix=f'Orbit-Build-{build}-{platform}/'
    if platform=='linux-x64':
        with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as z:
            safe_names(z.getnames())
            assert all(not m.issym() and not m.islnk() for m in z.getmembers())
            read=lambda n:z.extractfile(prefix+n).read()
            metadata=json.loads(read('BUILD.json'));source=read(f'Orbit-Build-{build}-Source.zip')
            assert read('Orbit')[:4]==b'\x7fELF'
            assert z.getmember(prefix+'Orbit').mode&0o111
    else:
        with zipfile.ZipFile(io.BytesIO(data)) as z:
            safe_names(z.namelist());assert z.testzip() is None
            metadata=json.loads(z.read(prefix+'BUILD.json'));source=z.read(prefix+f'Orbit-Build-{build}-Source.zip')
            executable='Orbit.exe' if platform=='windows-x64' else 'Orbit.app/Contents/MacOS/Orbit'
            assert len(z.read(prefix+executable))>1024
    assert metadata['build']==build and metadata['version']==version and metadata['platform']==platform
    with zipfile.ZipFile(io.BytesIO(source)) as z:
        safe_names(z.namelist());assert z.testzip() is None
        assert f'VERSION {version}' in z.read('CMakeLists.txt').decode()
        assert f'Build {build}  -  v{version}' in z.read('Source/Main.cpp').decode()
        assert not any(PurePosixPath(n).suffix.lower() in {'.exe','.dll','.dylib','.so','.o','.obj','.zip','.gz'}
                       or any(p in {'build','Builds','__pycache__'} or p.endswith(('.app','.vst3'))
                              for p in PurePosixPath(n).parts) for n in z.namelist())
        # Older Windows checkout artifacts use CRLF; compare the same UTF-8
        # source text across platforms while retaining every original ZIP byte.
        source_hashes={n:sha(z.read(n).replace(b'\r\n',b'\n')) for n in z.namelist() if not n.endswith('/')}
    return source,source_hashes

class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self,*args,**kwargs):
        return None

class GitHub:
    def __init__(self,repo,token):
        self.repo=repo;self.prefix=f'https://api.github.com/repos/{repo}'
        self.headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json',
                      'X-GitHub-Api-Version':'2022-11-28','User-Agent':'Orbit-tester-publication'}
    def api(self,path,method='GET',body=None):
        data=None if body is None else json.dumps(body).encode()
        request=urllib.request.Request(self.prefix+path,data=data,method=method,
            headers={**self.headers,'Content-Type':'application/json'})
        with urllib.request.urlopen(request,timeout=90) as response:return json.load(response)
    def download_artifact(self,artifact_id):
        request=urllib.request.Request(self.prefix+f'/actions/artifacts/{artifact_id}/zip',headers=self.headers)
        try:
            with urllib.request.build_opener(NoRedirect).open(request,timeout=90) as r:return r.read()
        except urllib.error.HTTPError as error:
            if error.code!=302:raise
            location=error.headers['Location']
        assert urllib.parse.urlparse(location).scheme=='https'
        # Download storage redirects without forwarding the GitHub token.
        with urllib.request.urlopen(location,timeout=90) as r:return r.read()
    def upload(self,release_id,name,data):
        url=f'https://uploads.github.com/repos/{self.repo}/releases/{release_id}/assets?'+urllib.parse.urlencode({'name':name})
        request=urllib.request.Request(url,data=data,method='POST',headers={**self.headers,
            'Content-Type':mimetypes.guess_type(name)[0] or 'application/octet-stream'})
        with urllib.request.urlopen(request,timeout=180) as response:return json.load(response)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest',type=Path)
    parser.add_argument('--artifact-folder',type=Path,help='Use downloaded wrapper ZIPs for offline verification')
    parser.add_argument('--verify-only',action='store_true')
    args=parser.parse_args();manifest=json.loads(args.manifest.read_text())
    repo=manifest['repository'];build=manifest['build'];version=manifest['version'];commit=manifest['source_commit']
    assert repo=='tjgoa/orbit-daw' and len(commit)==40
    assert len(manifest['artifacts'])==3
    github=None
    if not args.artifact_folder:
        github=GitHub(repo,os.environ['GITHUB_TOKEN'])
        run=github.api(f"/actions/runs/{manifest['run_id']}")
        assert run['status']=='completed' and run['conclusion']=='success' and run['head_sha']==commit
        assert run['path']=='.github/workflows/tester-builds.yml' and run['event']=='push'
        available={a['id']:a for a in github.api(f"/actions/runs/{manifest['run_id']}/artifacts?per_page=100")['artifacts']}
    assets={};source=None;source_hashes=None
    for item in manifest['artifacts']:
        platform=item['platform'];name=item['package']
        assert name==f'Orbit-Build-{build}-{platform}'+('.tar.gz' if platform=='linux-x64' else '.zip')
        if args.artifact_folder:wrapper=(args.artifact_folder/f'{platform}.zip').read_bytes()
        else:
            artifact=available[item['id']]
            assert not artifact['expired'] and artifact['name']==f'Orbit-{platform}-{commit}'
            assert artifact['workflow_run']['id']==manifest['run_id']
            assert artifact['digest']=='sha256:'+item['sha256']
            wrapper=github.download_artifact(item['id'])
        assert sha(wrapper)==item['sha256'], 'Artifact digest mismatch'
        with zipfile.ZipFile(io.BytesIO(wrapper)) as z:
            assert set(z.namelist())=={name,name+'.sha256'} and z.testzip() is None
            data=z.read(name);checksum=z.read(name+'.sha256')
            expected,filename=checksum.decode().strip().split()
            assert filename==name and sha(data)==expected, 'Package checksum mismatch'
            assets[name]=data;assets[name+'.sha256']=checksum
        current_source,current_hashes=inspect_package(data,build,version,platform)
        if source_hashes is None:source_hashes=current_hashes;source=current_source
        else:assert current_hashes==source_hashes, 'Platform source contents differ'
        print(f'Verified {name}: {len(data):,} bytes, SHA256 {sha(data)}',flush=True)
    assets[f'Orbit-Build-{build}-Source.zip']=source
    assets['INSTALL.md']=(ROOT/'DISTRIBUTION.md').read_bytes()
    assets['SHA256SUMS.txt']=''.join(f'{sha(data)}  {name}\n' for name,data in sorted(assets.items())).encode()
    print(f'All {len(assets)} release files verified.',flush=True)
    if args.verify_only:return
    assert github is not None, 'Offline input can only be verified'
    body=(ROOT/manifest['notes']).read_text()
    tag=manifest['tag']
    try:release=github.api('/releases/tags/'+urllib.parse.quote(tag,safe=''))
    except urllib.error.HTTPError as error:
        if error.code!=404:raise
        release=github.api('/releases','POST',{'tag_name':tag,'target_commitish':commit,
            'name':f'Orbit Build {build} / v{version} — Early tester builds','body':body,
            'draft':True,'prerelease':True,'make_latest':'false'})
    assert release['target_commitish']==commit and release['prerelease'], 'Unexpected existing release'
    existing={a['name']:a for a in github.api(f"/releases/{release['id']}/assets?per_page=100")}
    assert not (set(existing)-set(assets)), 'Release has unexpected existing files'
    for name,data in sorted(assets.items()):
        if name in existing:
            asset=existing[name]
            assert asset['size']==len(data) and asset['digest']=='sha256:'+sha(data), 'Existing asset differs; refusing overwrite'
        else:
            assert release['draft'], 'Cannot add missing files to an already-published release'
            asset=github.upload(release['id'],name,data)
            assert asset['size']==len(data) and asset['digest']=='sha256:'+sha(data)
        print('Uploaded/verified '+name,flush=True)
    if release['draft']:
        release=github.api(f"/releases/{release['id']}",'PATCH',{'draft':False,'body':body,'prerelease':True,'make_latest':'false'})
    assert not release['draft']
    print('Published '+release['html_url'],flush=True)
    summary=os.environ.get('GITHUB_STEP_SUMMARY')
    if summary:
        with open(summary,'a') as out:out.write(f"Published [{release['name']}]({release['html_url']}) with {len(assets)} verified assets. No product rebuild.\n")

if __name__=='__main__':
    main()
