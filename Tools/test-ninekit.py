#!/usr/bin/env python3
"""ASan/UBSan and allocation guards for the production 9W9 port; no product build."""
from pathlib import Path
import os,subprocess,tempfile
root=Path(__file__).resolve().parents[1];work=Path(tempfile.mkdtemp(prefix='orbit-nine-core-'))
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer'];objects=[]
for path in ['Source/Synth/NineKitVoice.c','Source/Synth/ThirdParty/9W9/9w9_engine.c']:
    obj=work/(Path(path).stem+'.o');objects.append(str(obj))
    subprocess.run(['clang','-std=c11','-O1']+san+['-Dmalloc=nine_checked_malloc','-Dcalloc=nine_checked_calloc','-Dfree=nine_checked_free','-c',str(root/path),'-o',str(obj)],check=True)
exe=work/'nine-test';subprocess.run(['clang++','-std=c++20','-O1']+san+[str(root/'Tools/test-ninekit.cpp')]+objects+['-o',str(exe)],check=True)
env=os.environ.copy();env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1';subprocess.run([str(exe)],check=True,env=env);print('Validation folder:',work)
