"""Shared local validation builds. No Orbit product build or numbering changes."""
from pathlib import Path
import hashlib
import os
import subprocess
import tempfile

IMPORT_SOURCES=['Source/Project/ProjectDocument.cpp','Source/Import/OrionSfsImportDialog.cpp',
    'Source/Import/OrionSfsConversion.cpp','Source/Import/ProjectImportAdapter.cpp',
    'Source/Import/ProjectImportSessionAdapter.cpp','Source/Import/ProjectImportPlan.cpp']

def compile_current_models(root,work,flags,sanitizers):
    objects={}
    for name in IMPORT_SOURCES:
        obj=work/(Path(name).stem+'.o')
        command=['/usr/bin/clang++']+sanitizers+flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+['-I',str(root/'Source'),'-c',str(root/name),'-o',str(obj)]
        log=work/(Path(name).stem+'-compile.log')
        with log.open('w') as output:result=subprocess.run(command,stdout=output,stderr=subprocess.STDOUT)
        if result.returncode:raise RuntimeError(log.read_text()[-9000:])
        objects['CMakeFiles/Orbit.dir/'+name+'.o']=str(obj)
    return objects

def acid_library(root,sanitizers):
    sources=[root/'Source/Synth/AcidBassVoice.cpp']+sorted((root/'Source/Synth/ThirdParty/Open303').glob('*.cpp'))
    inputs=sources+[root/'Source/Synth/AcidBassVoice.h']+list((root/'Source/Synth/ThirdParty/Open303').rglob('*.h'))
    digest=hashlib.sha256((' '.join(sanitizers)).encode())
    for source in sorted(inputs):digest.update(source.read_bytes())
    folder=Path(tempfile.gettempdir())/('orbit-acid-check-'+digest.hexdigest()[:16]);folder.mkdir(exist_ok=True)
    library=folder/'liborbit_acid_check.a'
    if library.exists():return library
    objects=[]
    for source in sources:
        obj=folder/(source.stem+'.o');objects.append(obj)
        with (folder/(source.stem+'.log')).open('w') as log:
            result=subprocess.run(['/usr/bin/clang++','-std=c++20','-O1']+sanitizers+['-c',str(source),'-o',str(obj)],stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise RuntimeError((folder/(source.stem+'.log')).read_text()[-9000:])
    temporary=library.with_suffix('.tmp.a')
    subprocess.run(['/usr/bin/ar','rcs',str(temporary)]+[str(o) for o in objects],check=True)
    temporary.replace(library)
    return library

def checked_environment():
    result=os.environ.copy();result['UBSAN_OPTIONS']='halt_on_error=1:print_stacktrace=1';return result


def native_clip_object(root,work,flags,sanitizers,native_probe=False):
    import sys
    if sys.platform!='darwin':return []
    obj=work/'NativeViewClip.o'
    args=['/usr/bin/clang++','-x','objective-c++']+sanitizers+flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+['-I',str(root/'Source'),'-c',str(root/'Source/Generator/NativeViewClip.mm'),'-o',str(obj)]
    with (work/'native-clip-compile.log').open('w') as log:result=subprocess.run(args,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise RuntimeError((work/'native-clip-compile.log').read_text()[-9000:])
    result=[str(obj),'-framework','QuartzCore']
    if native_probe:
        support=work/'native-support.o';command=args.copy();command[command.index(str(root/'Source/Generator/NativeViewClip.mm'))]=str(root/'Tools/native-view-test-support.mm');command[-1]=str(support)
        with (work/'native-support.log').open('w') as log:checked=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT)
        if checked.returncode:raise RuntimeError((work/'native-support.log').read_text()[-9000:])
        result += [str(support),'-framework','ImageIO']
    return result


def geonkick_library(root,sanitizers):
    vendor=root/'Source/Synth/ThirdParty/Geonkick'
    sources=[root/'Source/Synth/GeonkickVoice.c']+[vendor/(name+'.c') for name in ('oscillator','envelope','filter','distortion','gkick_buffer')]
    digest=hashlib.sha256((' '.join(sanitizers)).encode())
    for path in sorted(sources+list(vendor.rglob('*.h'))+[root/'Source/Synth/GeonkickVoice.h']):digest.update(path.read_bytes())
    folder=Path(tempfile.gettempdir())/('orbit-geonkick-check-'+digest.hexdigest()[:16]);folder.mkdir(exist_ok=True)
    library=folder/'liborbit_geonkick_check.a'
    if library.exists():return library
    objects=[]
    for source in sources:
        obj=folder/(source.stem+'.o');objects.append(obj)
        with (folder/(source.stem+'.log')).open('w') as log:
            result=subprocess.run(['/usr/bin/clang','-std=gnu11','-O1','-DORBIT_GEONKICK_SINGLE_OWNER=1','-DGEONKICK_VERSION_HEX=0x030900','-DGEONKICK_VERSION_STR="3.9.0"','-I',str(vendor/'quamplex_dsp_tools')]+sanitizers+['-c',str(source),'-o',str(obj)],stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise RuntimeError((folder/(source.stem+'.log')).read_text()[-9000:])
    subprocess.run(['/usr/bin/ar','rcs',str(library)]+[str(o) for o in objects],check=True)
    return library


def nine_library(root,sanitizers):
    vendor=root/'Source/Synth/ThirdParty/9W9'
    sources=[root/'Source/Synth/NineKitVoice.c',vendor/'9w9_engine.c']
    digest=hashlib.sha256((' '.join(sanitizers)).encode())
    for source in sorted(sources+list(vendor.glob('*.h'))+list((root/'Source/Synth').glob('NineKit*.h'))):digest.update(source.read_bytes())
    folder=Path(tempfile.gettempdir())/('orbit-nine-check-'+digest.hexdigest()[:16]);folder.mkdir(exist_ok=True)
    library=folder/'liborbit_nine_check.a'
    if library.exists():return library
    objects=[]
    for source in sources:
        obj=folder/(source.stem+'.o');objects.append(obj)
        with (folder/(source.stem+'.log')).open('w') as log:
            result=subprocess.run(['/usr/bin/clang','-std=c11','-O1']+sanitizers+['-c',str(source),'-o',str(obj)],stdout=log,stderr=subprocess.STDOUT)
        if result.returncode:raise RuntimeError((folder/(source.stem+'.log')).read_text()[-9000:])
    subprocess.run(['/usr/bin/ar','rcs',str(library)]+[str(o) for o in objects],check=True)
    return library
