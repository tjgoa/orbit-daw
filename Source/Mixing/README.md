# Mixer and Master Section — Builds 256–257

The complete installed `Mixer.htm` chapter was reviewed after Automation.
The channel strips already expose level/pan/mute/solo, stereo meters with peak
reset, four sends, inserts, four-band EQ and output routing.

Right-click an empty strip area or its bottom name label for **Copy Channel
Settings**, **Paste Channel Settings**, **Copy EQ Settings**, **Paste EQ Settings**
and **Rename Channel**. Full settings copy level, pan, mute/solo, EQ values/bypass
and generator sends/link modes. EQ-only paste changes only EQ. Pasting is one
project Undo transaction. Instruments, insert effects, output destinations,
notes and event lanes are retained. The clipboard contains values and works
across projects for this session; it is not a plugin-state/effect clipboard.
Return/bus names are editable as well as generator names.

Right-click an FX send for **Linked to Channel Volume**. Linked sends are after
the native volume/pan stage; unlinked sends are before it. An inactive send
remembers the choice before it is created. This setting survives save/reopen and
is undoable. Orbit currently combines gain and pan, so this also changes the
send's pan dependency. Orion's distinct gain-before-pan versus pan-before-gain
tap points are not reproduced by this first implementation. Tooltips identify
pre/post-fader behavior. There is no hidden attempt to cancel gain in the DSP.

EQ knobs (compact strips and full EQ window) now offer parameter-bound Pattern
and Song events. Sends offer the same once created by adjusting their level.
Master and return/bus controls use Song lanes; Pattern entry is generator-only.
External plugin GUI context menus stay owned by that plugin.

## Build 257: output routing and Master Section

Click **Master** and **1–8** route buttons independently: multiple destinations
may be enabled together. The dry signal reaches each selected destination once.
Bus-only routing excludes direct Master audio. Switching all destinations off
silences the final dry branch while retaining FX sends. A private output gate
avoids the engine's empty-output-name fallback to Master. Routing uses reserved
aux buses 32–39, separate from FX sends 0–7; private routing controls are hidden
from instrument/effect and event menus. Direct and auxiliary feedback cycles are
refused without changing the project. Each route edit is one Undo operation.

New projects have four shared FX returns (reverb, delay, chorus and phaser).
Opening Master Section adds any missing factory returns in older projects and
marks the project changed. Output buses appear only while another channel routes
to them; their state remains retained when hidden. Returns and buses can use the
same routing controls, with feedback protection.

New projects place **Master volume before EQ and insert effects**, matching the
manual's mastering workflow. Existing projects keep their saved order. Right-click
Master's name/empty strip and toggle **Volume Before Insert Effects** to change
it. The host moves volume/pan state and native curves, and migrates plugin plus
parameter identities in Song lanes, including unambiguous legacy lanes. One Undo
restores the previous order/state. The engine's final Master stage stays neutral
while the pre-insert fader is active, so gain is not applied twice.

Right-click an EQ knob and toggle **Enable Soft Saturation** for Orbit's optional
symmetric soft saturation immediately after EQ. EQ bypass suspends that stage
while remembering the preference. Channel/EQ settings copy the option too.
This is Orbit's own response; it is not an exact model of Orion's proprietary EQ.

New saves use schema 10 and require Build 257 or newer. Earlier supported projects
still load. See Source/Project/PROJECT_FORMAT.md for routing/private-plugin state.

## Remaining Mixer details

- Orion's separate gain/pan send tap order (the current choice changes both).
- Exact legacy EQ frequency/response calibration.
- Multi-output generator expansion; current generator strips are stereo.
- Effect-state copy/paste and hardware output assignments.

These remain explicit gaps; the chapter is not claimed complete. Native
proprietary Orion DSP is not copied. Multi-output/hardware tests require suitable
plugins/devices; the separate send tap architecture remains a later routing pass.

Validation: `python3 Tools/test-options-host.py --sanitize --mixer` covers settings
copy/paste, Undo/Redo, destination data preservation, return rename and fresh
reopen. Real native graph renders measure parallel Master/bus sums, bus-only
isolation, multiple buses, pre/post-fader sends and disconnected dry output,
Master volume across a nonlinear compressor, and EQ saturation/bypass. Tests also
cover feedback refusal, global automation identity migration, four returns and
unused-bus visibility. Device timing and human listening remain manual checks.


## Build 258: sidechains and automation-safe Master mute

The effect parameter window shows a **Sidechain** selector for effects that
expose sidechain inputs. Choose a generator/audio/return/bus channel or None.
Native Compressor automatically enables its external trigger while connected.
The source label updates immediately. Sources that would create direct, aux or
sidechain feedback are disabled/refused. Changes use one Undo transaction and
persist by engine IDs. Missing source IDs remain visible as unavailable until
you choose another source or None. Vendor-native GUI menus remain vendor-owned.

Native render checks measure destination ducking and recovery, source-level
response and disconnect, with separate source/destination tone frequencies.
The selector is general, but only the native Compressor is newly render-certified;
particular VST sidechain layouts still need plugin-specific testing. Orion's
Programmable Gate, sidechain filter and ring modulator DSP remain future effects.

Host generic parameter sliders and Mixer controls now read native values during
automation playback. Refreshes do not write parameter edits and pause during a
control drag, then resume after release.

Master mute is now a separate final gate: volume automation continues silently
rather than overriding a manually lowered fader. Unmute retains the curves and
current level. New Master inserts stay ahead of the gate. Legacy muted projects
recover their stored pre-mute level behind the new gate. Native renders verify
silence during level automation, unmute and fresh reopen. New saves use schema 11
and require Build 258 or newer.


Build 261 adds an original native sample-peak look-ahead limiter. See
[LIMITER.md](LIMITER.md) for controls, scope, native latency/export tests and
manual listening checks. This does not complete the remaining Dynamics family.


Build 263 adds the original Peak Compressor, covering the CV-82 manual's general
control set without claiming a sonic clone. See [PEAK_COMPRESSOR.md](PEAK_COMPRESSOR.md)
for controls, detector/latency choices, native tests and comparison candidates.
Host parameter sliders respect skew and group a drag into one Orbit Undo. The
native export offset overlay fixes start-of-file latency trimming for all effects.


Build 267 adds original native Tube and Two-Band Distortion with 4x FIR processing,
latency-aligned blend and native automation/state. See [DISTORTION.md](DISTORTION.md)
for controls/checks and [DISTORTION_RESEARCH.md](DISTORTION_RESEARCH.md) for MIT
candidates and the limits of the ABL2 identification. No sonic clone is claimed.
