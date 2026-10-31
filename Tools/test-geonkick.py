#!/usr/bin/env python3
"""Sanitizer and real-time guard test of the exact Geonkick audition sources.
No product build, build-number reservation, audio hardware or plugin install.
Requires clang/clang++ and Python 3; outputs disposable objects under /tmp.
"""
from pathlib import Path
import os
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
vendor=root/'Source/Synth/ThirdParty/Geonkick'
work=Path(tempfile.mkdtemp(prefix='orbit-geonkick-rt-'))
san=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
objects=[]
sources=[root/'Source/Synth/GeonkickVoice.c']+[vendor/(n+'.c') for n in ('oscillator','envelope','filter','distortion','gkick_buffer')]
for source in sources:
    obj=work/(source.stem+'.o');objects.append(str(obj))
    subprocess.run(['clang','-std=gnu11','-O1']+san+[
        '-DORBIT_GEONKICK_SINGLE_OWNER=1','-DGEONKICK_VERSION_HEX=0x030900',
        '-Dmalloc=orbit_checked_malloc','-Dcalloc=orbit_checked_calloc',
        '-Dfree=orbit_checked_free','-Dpthread_mutex_lock=orbit_checked_lock',
        '-Dpthread_mutex_unlock=orbit_checked_unlock','-I',str(vendor/'quamplex_dsp_tools'),
        '-c',str(source),'-o',str(obj)],check=True)
exe=work/'drum-core-test'
subprocess.run(['clang++','-std=c++20','-O1','-DORBIT_GEONKICK_RT_GUARD=1']+san+
    [str(root/'Tools/test-geonkick.cpp')]+objects+['-o',str(exe)],check=True)
env=os.environ.copy();env['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1'
subprocess.run([str(exe)],env=env,check=True)
print('Validation folder:',work)
