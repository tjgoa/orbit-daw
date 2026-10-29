#!/usr/bin/env python3
"""Verify pinned dependencies and apply Orbit's small sampler overlay once."""
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[1]
engine=root/'Dependencies/tracktion_engine'
for path,expected in [(engine,'0a5f4e6a5f53d09c89b414a44386a12df7fa1ec6'),(engine/'modules/juce','19edd538429c93d277bf95b55aaa7e3eb545f951')]:
    actual=subprocess.check_output(['git','-C',str(path),'rev-parse','HEAD'],text=True).strip()
    if actual!=expected:raise SystemExit(f'Wrong dependency revision at {path.name}: {actual}; expected {expected}')
patch=root/'Tools/tracktion-sampler.patch'
def check(*args):return subprocess.run(['git','-C',str(engine),'apply','--check',*args,str(patch)],capture_output=True).returncode==0
if check('--reverse'):print('Orbit sampler overlay already applied.')
elif check():subprocess.run(['git','-C',str(engine),'apply',str(patch)],check=True);print('Applied Orbit sampler overlay.')
else:raise SystemExit('Sampler overlay conflicts with existing engine changes; no files changed.')
