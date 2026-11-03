#!/usr/bin/env python3
"""Embed the native JUCE atlas output in a self-contained local HTML explorer."""
from pathlib import Path
import sys,json,base64
here=Path(__file__).resolve().parent
folder=Path(sys.argv[1]);presets=json.loads((folder/'knobs.json').read_text())
assert len(presets)==33
for p in presets:
    p['atlases']={size:'data:image/png;base64,'+base64.b64encode((folder/name).read_bytes()).decode() for size,name in p['atlases'].items()}
output=here/'Orbit-Knob-Explorer.html';output.write_text((here/'explorer-template.html').read_text().replace('__PRESETS__',json.dumps(presets,separators=(',',':'))))
print(f'{output}: {output.stat().st_size:,} bytes, {len(presets)} native styles')
