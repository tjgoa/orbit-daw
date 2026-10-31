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
import json
import plistlib
import shutil
from check_build_support import compile_current_models,acid_library,geonkick_library,nine_library,checked_environment,native_clip_object

parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--test-file',type=Path);parser.add_argument('--compile-only',action='store_true');parser.add_argument('--sanitize',action='store_true');parser.add_argument('--generator-controls',action='store_true');parser.add_argument('--distortion',action='store_true');parser.add_argument('--peak-compressor',action='store_true');parser.add_argument('--limiter',action='store_true');parser.add_argument('--effects',action='store_true');parser.add_argument('--effect-plugin',type=Path);parser.add_argument('--instrument-plugin',type=Path);parser.add_argument('--sidechain',action='store_true');parser.add_argument('--mixer',action='store_true');parser.add_argument('--automation',action='store_true');parser.add_argument('--piano-roll',action='store_true');parser.add_argument('--generator',action='store_true');parser.add_argument('--recording',action='store_true');parser.add_argument('--playlist',action='store_true');parser.add_argument('--patterns',action='store_true');parser.add_argument('--acid',action='store_true');parser.add_argument('--sfs-file',type=Path);parser.add_argument('--native-plugins',nargs='+',type=Path);options=parser.parse_args()
root = Path(__file__).resolve().parents[1]
work = Path(tempfile.mkdtemp(prefix='orbit-options-check-'))
source = (root/'Source/Main.cpp').read_text()
source = source.replace('private:', 'public:').replace('private juce::Timer', 'public juce::Timer')
source = source.replace('centreWithSize(', 'setSize(')
plugin_editor=(root/'Source/Generator/PluginEditorContent.h').read_text().replace('private:', 'public:')
plugin_editor=plugin_editor.replace('#include "NativeViewClip.h"','#include "'+str(root/'Source/Generator/NativeViewClip.h')+'"')
source=source.replace('#include "Generator/PluginEditorContent.h"',plugin_editor)

song_editor = (root/'Source/Automation/SongEventEditor.h').read_text().replace('centreWithSize(', 'setSize(')
for relative in ('../Project/ProjectDocument.h', '../Editing/EditShortcuts.h', '../Workspace/WorkspaceWindow.h'):
    song_editor = song_editor.replace(relative, str((root/'Source/Automation'/relative).resolve()))
(work/'SongEventEditorCheck.h').write_text(song_editor)
source = source.replace('#include "Automation/SongEventEditor.h"', '#include "SongEventEditorCheck.h"')
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
static int checkedWorkspaceLayouts = 0;
static bool checkedSongEventFocus = false;
static std::unique_ptr<orbit::generator::ArpeggiatorPanel> checkedArpeggiator;
static std::function<bool(orbit::generator::Settings,bool)> checkedArpeggiatorCommit;
static juce::Component* checkedGeneratorMenuAnchor = nullptr;
static std::function<void(int)> checkedGeneratorMenuResult;
'''+source[pos:]
layout_marker='    void resized() override\n    {\n        auto menuRow = getLocalBounds().removeFromTop(28).reduced(4, 2);'
assert source.count(layout_marker)==1
source=source.replace(layout_marker,layout_marker.replace('        auto menuRow','        ++checkedWorkspaceLayouts;\n        auto menuRow'))
# Headless focus substitute: production menu enablement/dispatch still runs.
source=source.replace('songEventWindow->panel.hasKeyboardFocus(true)','(checkedSongEventFocus || songEventWindow->panel.hasKeyboardFocus(true))')
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
source += options.test_file.read_text() if options.test_file else (root/'Tools'/('test-distortion-host.cpp' if options.distortion else 'test-peak-compressor-host.cpp' if options.peak_compressor else 'test-limiter-host.cpp' if options.limiter else 'test-generator-controls-host.cpp' if options.generator_controls else 'test-effects-host.cpp' if options.effects else 'test-sidechain-host.cpp' if options.sidechain else 'test-mixer-host.cpp' if options.mixer else 'test-automation-host.cpp' if options.automation else 'test-native-generator-host.cpp' if options.native_plugins else 'test-acid-host.cpp' if options.acid else 'test-pattern-editor-host.cpp' if options.patterns else 'test-playlist-host.cpp' if options.playlist else 'test-recording-host.cpp' if options.recording else 'test-generator-host.cpp' if options.generator else 'test-piano-roll-host.cpp' if options.piano_roll else 'test-options-host.cpp')).read_text()
source = source.replace('#include "test-event-workspace-host.cpp"', (root/'Tools/test-event-workspace-host.cpp').read_text())
source = source.replace('#include "test-effect-workspaces-host.cpp"', (root/'Tools/test-effect-workspaces-host.cpp').read_text())
source = source.replace('#include "test-mixer-saturation-host.cpp"', (root/'Tools/test-mixer-saturation-host.cpp').read_text())
source = source.replace('#include "test-mixer-routing-host.cpp"', (root/'Tools/test-mixer-routing-host.cpp').read_text())
source = source.replace('#include "test-playlist-selection-host.cpp"', (root/'Tools/test-playlist-selection-host.cpp').read_text())
source = source.replace('#include "test-recording-loop-host.cpp"', (root/'Tools/test-recording-loop-host.cpp').read_text())
source = source.replace('#include "test-pattern-clipboard-host.cpp"', (root/'Tools/test-pattern-clipboard-host.cpp').read_text())
source = source.replace('OPTIONS_CHECK_FOLDER', str(work))
source = source.replace('GENERATOR_VST_PATH_LITERAL',json.dumps(str(options.instrument_plugin.resolve()) if options.instrument_plugin else ''))
source = source.replace('VST_EFFECT_PATH_LITERAL',json.dumps(str(options.effect_plugin.resolve()) if options.effect_plugin else ''))
source = source.replace('NATIVE_PLUGIN_PATHS',','.join(json.dumps(str(p.resolve())) for p in (options.native_plugins or [])))
source = source.replace('SFS_CHECK_SOURCE_LITERAL',json.dumps(str(options.sfs_file.resolve()) if options.sfs_file else ''))
cpp = work/'options-host.cpp';cpp.write_text(source)
flags = {}
for line in (root/'build/make/CMakeFiles/Orbit.dir/flags.make').read_text().splitlines():
    if ' = ' in line:
        key,value = line.split(' = ',1); flags[key] = shlex.split(value)
# Keep source locations for sanitizer stacks without duplicating the full
# application's debug type graph in every disposable validation object/binary.
flags['CXX_FLAGS'] += ['-gline-tables-only']
obj = work/'options-host.o';exe = work/'options-host'
sanitizers=['-fsanitize=address,undefined','-fno-omit-frame-pointer'] if options.sanitize else []
args = ['/usr/bin/clang++']+sanitizers+flags['CXX_DEFINES']+flags['CXX_INCLUDES']+flags['CXX_FLAGS']+['-I',str(root/'Source'),'-I',str(root/'Tools'),'-I',str(work),'-c',str(cpp),'-o',str(obj)]
print('Check folder:',work,flush=True)
with (work/'compile.log').open('w') as log:
    result = subprocess.run(args,stdout=log,stderr=subprocess.STDOUT)
if result.returncode:
    print((work/'compile.log').read_text()[-15000:]);raise SystemExit(result.returncode)
# Compile current model/import layouts together; stale ABI objects can hide bugs.
try:
    current_objects=compile_current_models(root,work,flags,sanitizers)
    dsp_library=acid_library(root,sanitizers)
    drum_library=geonkick_library(root,sanitizers)
    nine_dsp=nine_library(root,sanitizers)
    native_objects=native_clip_object(root,work,flags,sanitizers,options.native_plugins is not None)
except RuntimeError as error:print(error);raise SystemExit(1)
args = shlex.split((root/'build/make/CMakeFiles/Orbit.dir/link.txt').read_text())
args[args.index('CMakeFiles/Orbit.dir/Source/Main.cpp.o')] = str(obj)
for name,obj in current_objects.items():args[args.index(name)]=obj
args.extend([str(dsp_library),str(drum_library),str(nine_dsp)]);args+=native_objects
args[args.index('-o')+1] = str(exe)
args += sanitizers
with (work/'link.log').open('w') as log:
    result = subprocess.run(args,cwd=root/'build/make',stdout=log,stderr=subprocess.STDOUT)
if result.returncode:
    print((work/'link.log').read_text()[-5000:]);raise SystemExit(result.returncode)
if options.native_plugins:
    bundle=work/'Orbit UI Verification.app'
    (bundle/'Contents/MacOS').mkdir(parents=True)
    target=bundle/'Contents/MacOS/OrbitUIVerification';shutil.move(exe,target);exe=target
    (bundle/'Contents/Info.plist').write_bytes(plistlib.dumps(dict(CFBundleExecutable=target.name,CFBundleIdentifier='com.touchtheuniverse.orbit.nativeverification',CFBundleName='Orbit UI Verification',CFBundlePackageType='APPL',NSPrincipalClass='NSApplication',NSHighResolutionCapable=True)))
    subprocess.run(['codesign','--force','--sign','-',str(bundle)],check=True)
if options.compile_only:
    print('Compiled verification executable:',exe,flush=True)
    raise SystemExit(0)
with (work/'result.log').open('w') as log:
    result = subprocess.run([str(exe)],stdout=log,stderr=subprocess.STDOUT,timeout=300 if options.native_plugins else 180 if options.distortion else 180 if options.patterns or options.acid else 60,env=checked_environment())
print((work/'result.log').read_text()[-6500:])
if result.returncode: raise SystemExit(result.returncode)
if options.distortion:
    result=subprocess.run([str(exe),'--verify-distortion'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.peak_compressor:
    result=subprocess.run([str(exe),'--verify-peak-compressor'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.limiter:
    result=subprocess.run([str(exe),'--verify-limiter'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.effects:
    result=subprocess.run([str(exe),'--verify-effects'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.sidechain:
    result=subprocess.run([str(exe),'--verify-sidechain'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.mixer:
    result=subprocess.run([str(exe),'--verify-mixer-settings'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.automation:
    result=subprocess.run([str(exe),'--verify-song-events'],timeout=60,env=checked_environment())
    if result.returncode:raise SystemExit(result.returncode)
if options.acid:
    for action in ('--verify-acid-project','--verify-missing-project'):
        result=subprocess.run([str(exe),action],timeout=60,env=checked_environment())
        if result.returncode:raise SystemExit(result.returncode)
if options.recording:
    result=subprocess.run([str(exe),'--verify-record-count-in'],timeout=60)
    if result.returncode:raise SystemExit(result.returncode)
if options.playlist:
    result=subprocess.run([str(exe),'--verify-scroll-policy'],timeout=60)
    if result.returncode:raise SystemExit(result.returncode)
if options.patterns:
    result=subprocess.run([str(exe),'--verify-pattern-project'],timeout=60)
    if result.returncode:raise SystemExit(result.returncode)
if options.piano_roll:
    result=subprocess.run([str(exe),'--verify-scale-policy'],timeout=60)
    if result.returncode:raise SystemExit(result.returncode)
for expected,next_skin in ([] if options.distortion or options.peak_compressor or options.limiter or options.generator_controls or options.effects or options.sidechain or options.test_file or options.mixer or options.automation or options.native_plugins or options.acid or options.patterns or options.playlist or options.piano_roll or options.generator or options.recording else [(5,6),(6,None)]):
    command=[str(exe),'--verify-skin',str(expected)]+([str(next_skin)] if next_skin is not None else [])
    with (work/('restore-'+str(expected)+'.log')).open('w') as log:
        result=subprocess.run(command,stdout=log,stderr=subprocess.STDOUT,timeout=60)
    print((work/('restore-'+str(expected)+'.log')).read_text()[-1800:])
    if result.returncode: raise SystemExit(result.returncode)

