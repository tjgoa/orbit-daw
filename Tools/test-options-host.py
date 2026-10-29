#!/usr/bin/env python3
"""Exercise production Options or Piano Roll handlers with isolated settings and no audio hardware.
Requires an existing CMake Orbit build; writes disposable checks only in /tmp.
The harness substitutes dialog presentation, app startup, display centering and
engine/settings storage. Production menu, routing, panels and edit handlers run.
"""
from pathlib import Path
import argparse
import re
import shlex
import subprocess
import tempfile

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--sanitize',action='store_true');parser.add_argument('--piano-roll',action='store_true');parser.add_argument('--generator',action='store_true');parser.add_argument('--recording',action='store_true');options=parser.parse_args()
root = Path(__file__).resolve().parents[1]
work = Path(tempfile.mkdtemp(prefix='orbit-options-check-'))
source = (root/'Source/Main.cpp').read_text()
source = source.replace('private:', 'public:').replace('private juce::Timer', 'public juce::Timer')
source = source.replace('centreWithSize(', 'setSize(')
source = source.replace('#include "Generator/GeneratorHostMethods.h"', (root/'Source/Generator/GeneratorHostMethods.h').read_text())
source = re.sub(r'START_JUCE_APPLICATION\([^\n]+\)', '', source)
source = source.replace('#include "Options/VirtualKeyboard.h"', '#include "VirtualKeyboardCheck.h"')
(work/'VirtualKeyboardCheck.h').write_text((root/'Source/Options/VirtualKeyboard.h').read_text().replace('private:', 'public:').replace('centreWithSize(', 'setSize('))
pos = source.index('\nnamespace')
source = source[:pos]+'''\n#include <iostream>
#include <stdexcept>
static std::unique_ptr<orbit::options::Panel> checkedOptions;
static std::function<bool(const orbit::options::Panel&)> checkedApply;
static int checkedKeyboardToggles = 0;
static std::unique_ptr<orbit::generator::ArpeggiatorPanel> checkedArpeggiator;
static std::function<bool(orbit::generator::Settings,bool)> checkedArpeggiatorCommit;
static juce::Component* checkedGeneratorMenuAnchor = nullptr;
static std::function<void(int)> checkedGeneratorMenuResult;
'''+source[pos:]
pos = source.index('class ProjectView final')
storage = '''struct OptionsCheckStorage : te::PropertyStorage {
    explicit OptionsCheckStorage(const juce::String& path):PropertyStorage("OrbitOptionsCheck"),folder(path){}
    juce::File getAppCacheFolder() override {return folder.getChildFile("cache");}
    juce::File getAppPrefsFolder() override {return folder.getChildFile("engine-prefs");}
    juce::File folder;
};
'''
source = source[:pos]+storage+source[pos:]
source = source.replace('ProjectView() : engine("Orbit",', 'ProjectView() : engine(std::make_unique<OptionsCheckStorage>("'+str(work)+'"),', 1)
source = source.replace('class OrbitAudioEngineBehaviour final : public te::EngineBehaviour\n{\npublic:', 'class OrbitAudioEngineBehaviour final : public te::EngineBehaviour\n{\npublic:\n    bool autoInitialiseDeviceManager() override { return false; }', 1)
source = source.replace('settingsOptions.applicationName = "Orbit";', 'settingsOptions.applicationName = "OrbitOptionsCheck";\n        settingsOptions.folderName = "'+str(work)+'/prefs";', 1)
a = source.index('    void openOptionsTool(')
b = source.index('    juce::PopupMenu makeOptionsMenu()', a)
source = source[:a]+'''    void openOptionsTool(const juce::String&,std::unique_ptr<OptionsPanel> panel,
        std::function<bool(const OptionsPanel&)> action,bool=false)
    {checkedOptions=std::move(panel);checkedApply=std::move(action);}
'''+source[b:]
a = source.index('    void toggleVirtualKeyboard()')
b = source.index('    void setMidiOutputEnabled(', a)
source = source[:a]+'''    void toggleVirtualKeyboard() {++checkedKeyboardToggles;}
'''+source[b:]
a = source.index('void presentGeneratorArpeggiator(')
b = source.index('void showGeneratorArpeggiator(', a)
source = source[:a]+'''void presentGeneratorArpeggiator(std::unique_ptr<orbit::generator::ArpeggiatorPanel> panel,
    std::function<bool(orbit::generator::Settings,bool)> commit)
{checkedArpeggiator=std::move(panel);checkedArpeggiatorCommit=std::move(commit);}
'''+source[b:]
a = source.index('void presentGeneratorMenu(')
b = source.index('void showGeneratorMenuFor(', a)
source = source[:a]+'''void presentGeneratorMenu(juce::PopupMenu,juce::Component* anchor,std::function<void(int)> result)
{checkedGeneratorMenuAnchor=anchor;checkedGeneratorMenuResult=std::move(result);}
'''+source[b:]
source += (root/'Tools'/('test-recording-host.cpp' if options.recording else 'test-generator-host.cpp' if options.generator else 'test-piano-roll-host.cpp' if options.piano_roll else 'test-options-host.cpp')).read_text()
source = source.replace('OPTIONS_CHECK_FOLDER', str(work))
cpp = work/'options-host.cpp';cpp.write_text(source)
flags = {}
for line in (root/'build/make/CMakeFiles/Orbit.dir/flags.make').read_text().splitlines():
    if ' = ' in line:
        key,value = line.split(' = ',1); flags[key] = shlex.split(value)
obj = work/'options-host.o';exe = work/'options-host'
sanitizers=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if options.sanitize else []
args = ['/usr/bin/clang++']+sanitizers+flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+['-I',str(root/'Source'),'-I',str(work),'-c',str(cpp),'-o',str(obj)]
print('Check folder:',work,flush=True)
with (work/'compile.log').open('w') as log:
    result = subprocess.run(args,stdout=log,stderr=subprocess.STDOUT)
if result.returncode:
    print((work/'compile.log').read_text()[-15000:]);raise SystemExit(result.returncode)
args = shlex.split((root/'build/make/CMakeFiles/Orbit.dir/link.txt').read_text())
args[args.index('CMakeFiles/Orbit.dir/Source/Main.cpp.o')] = str(obj)
args[args.index('-o')+1] = str(exe)
args += sanitizers
with (work/'link.log').open('w') as log:
    result = subprocess.run(args,cwd=root/'build/make',stdout=log,stderr=subprocess.STDOUT)
if result.returncode:
    print((work/'link.log').read_text()[-5000:]);raise SystemExit(result.returncode)
with (work/'result.log').open('w') as log:
    result = subprocess.run([str(exe)],stdout=log,stderr=subprocess.STDOUT,timeout=60)
print((work/'result.log').read_text()[-6500:])
if result.returncode: raise SystemExit(result.returncode)
for expected,next_skin in ([] if options.piano_roll or options.generator or options.recording else [(5,6),(6,None)]):
    command=[str(exe),'--verify-skin',str(expected)]+([str(next_skin)] if next_skin is not None else [])
    with (work/('restore-'+str(expected)+'.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=60)
    print((work/('restore-'+str(expected)+'.log')).read_text()[-1800:])
    if result.returncode: raise SystemExit(result.returncode)

