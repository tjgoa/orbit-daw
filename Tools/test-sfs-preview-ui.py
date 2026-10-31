#!/usr/bin/env python3
"""Exercise the production SFS dialog with a local Song1 fixture, without opening a window or merging into a user session."""
import argparse
from pathlib import Path
import shlex
import subprocess
import tempfile
from check_build_support import compile_current_models,checked_environment

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('song1',type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1]
fixture=args.song1.resolve();assert fixture.is_file()
work=Path(tempfile.mkdtemp(prefix='orbit-sfs-ui-check-'))
flags={}
for line in (root/'build/make/CMakeFiles/Orbit.dir/flags.make').read_text().splitlines():
    if ' = ' in line:
        key,value=line.split(' = ',1);flags[key]=shlex.split(value)
sanitize=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
print('Check folder:',work,flush=True)
objects=[]
for name,path in [('test',root/'Tools/test-sfs-preview-ui.cpp')]:
    obj=work/(name+'.o');objects.append(obj)
    command=['/usr/bin/clang++']+sanitize+flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+['-I',str(root/'Source/Import'),'-c',str(path),'-o',str(obj)]
    with (work/(name+'-compile.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        print((work/(name+'-compile.log')).read_text()[-9000:]);raise SystemExit(result.returncode)
command=shlex.split((root/'build/make/CMakeFiles/Orbit.dir/link.txt').read_text())
command[command.index('CMakeFiles/Orbit.dir/Source/Main.cpp.o')]=str(objects[0])
current=compile_current_models(root,work,flags,sanitize)
for name,obj in current.items():command[command.index(name)]=obj
command[command.index('-o')+1]=str(work/'sfs-ui-check');command+=sanitize
with (work/'link.log').open('w') as log:
    result=subprocess.run(command,cwd=root/'build/make',stdout=log,stderr=subprocess.STDOUT)
if result.returncode:
    print((work/'link.log').read_text()[-5000:]);raise SystemExit(result.returncode)
with (work/'result.log').open('w') as log:
    result=subprocess.run([str(work/'sfs-ui-check'),str(fixture),str(work)],stdout=log,stderr=subprocess.STDOUT,timeout=60,env=checked_environment())
print((work/'result.log').read_text()[-5000:]);raise SystemExit(result.returncode)
