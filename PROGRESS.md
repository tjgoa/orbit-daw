# Orbit development progress

## 2026-10-30 — Build 274 / v0.3.253 Triangle Foldback

Acid Bass Effects / Drive now has a three-choice shape selector: Soft, Sine Fold
and the user-selected Triangle Foldback. The new original piecewise-linear
folding transfer shares the existing 4x oversampling, pre-filter, post-tone/DC
rejection, compensated dry/wet blend and Output. Shape changes use 5 ms
smoothing. Distort 0% stays dry. No new third-party code is adopted.

A new native fxTriangleFold boolean defaults Off and overrides the old shaping
only when enabled. The fxMode ID and its 0..1 normalized range are retained for
Soft/Sine Fold and saved curves. The selector changes the appropriate controls
with one Undo; selecting Triangle retains the old mode/curve for return to the
original shapes. Schema 20 protects the new field. Build 273's per-instance
phase-retrigger Settings option is included unchanged.

ASan/UBSan host checks pass for triangle-vs-existing shapes, 44.1/48/96 kHz dry
alignment, blend, segmentation, extremes and transition stability; native
rendered Triangle audio and its automation; selector/Undo, legacy defaults and
mode-curve range; restored selector/state and fresh-process render in Song mode.
Phase tests and original FX/transport/window regressions pass. Listening,
hardware CPU tests and actual Windows/Linux builds remain manual.

Build 274 / v0.3.253 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 273 is preserved at Builds/Previous/Orbit_Build_273_before_274.app. The running app was not closed. Successful development iteration 303.

## 2026-10-30 — Build 273 / v0.3.252 Acid Bass phase retrigger

The existing generator dropdown > Generator Settings now has an Acid Bass-only
"Retrigger oscillator phase (slides stay continuous)" checkbox. Apply targets
the generator that opened the dialog even if another generator is selected.
The per-instance native parameter phaseRetrigger defaults Off for new/legacy
state, resets oscillator phase to zero on fresh attacks, and leaves outgoing
Slide ties/live overlapping legato continuous. It changes the next attack,
without resetting a held note or clearing filter/envelope history.

The native parameter is appended after the existing fifteen Acid Bass parameters;
all original IDs/ranges, including normalized Soft/Sine Fold automation, stay
unchanged. Settings-only and combined MIDI/phase edits group under one Undo,
mark dirty, restore after reopen and refuse a stale dialog after project change.
Schema 19 protects the new setting from older readers. Other generators keep
their existing MIDI Settings fields.

ASan/UBSan checks cover both waveforms at five rates (8–192 kHz), fresh attacks,
original idle-note identity, pattern slides and live legato identity, and Panic.
Production native host checks cover exact MIDI positions/stereo/buffer bounds,
phase On/Off, legacy Off, settings target/Apply/Undo/Redo, combined MIDI edits,
instance isolation and project persistence/render. Existing Acid FX multirate
DSP/dry latency/blend/automation and transport/window regression passes.
Listening and hardware CPU timing remain manual. Distortion suggestions are
asymmetric drive, hard clipping and triangle foldback; existing shapes remain
Soft/Sine Fold in this iteration. See Source/Synth/ACID_BASS_FX.md.

Build 273 / v0.3.252 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 272 is preserved at Builds/Previous/Orbit_Build_272_before_273.app. The running app was not closed. Successful development iteration 302.

## 2026-10-30 — Build 272 / v0.3.251 9W9 909 audition

Insert > Drum Auditions > 9W9 909 Audition adds an eleven-sound native kit using
pinned v2.7.2 GPL-3.0 upstream DSP and resources. The native editor exposes 98
parameters over drum/Master/Reverb/Delay pages, a paged 16-step view, per-step
velocity and whole-kit Accent, audition hits and a protected four-bar demo.
Notes, controls and native automation persist in schema 18. Multiple instances
keep separate engine state, open panels and position through Play/Stop.

9W9 by A Thousand Details - https://github.com/athousanddetails/schwung-9W9

This is a modified Orbit port, not the original and not made or endorsed by
A Thousand Details. LICENSE/NOTICE remain unaltered, exact attribution and the
modified statement appear in About/Credits/docs. Original source/sample hashes,
rate correction, compiled PCM and wrapper changes are recorded in UPSTREAM.md.
This is one stereo output; full Pro-9 pan/controls and multiple outputs remain.

ASan/UBSan core checks pass for all eleven voices at five rates (22.05–192 kHz),
velocity/step accent, hat choke, sample pitch across rates, block invariance,
instance isolation, extremes, and silent panic. Callback paths allocate/free
zero times. Native host checks pass for exact MIDI timing/Accent ordering,
step paging/lights, knob gestures/Undo/Event targets, protected demo, GM/pad maps,
actual audio automation and preview, independent windows, Pattern/Song transport,
and fresh-process save/reopen/render. Geonkick/ChowKick native regression passes.
Listening, hardware CPU tests and Windows/Linux builds remain manual.

Build 272 / v0.3.251 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 271 is preserved at Builds/Previous/Orbit_Build_271_before_272.app. The running app was not closed. Successful development iteration 301.

## 2026-10-30 — Build 271 / v0.3.250 Drum engine auditions

Insert > Drum Auditions > Geonkick Audition runs pinned GPLv3-or-later upstream
oscillator, envelope, filter and distortion DSP through a small Orbit interface.
Six Orbit sound recipes (Kick/Snare/Hat/Tom/Clap/Metal), seven knobs, drag values,
Hear hit and a protected empty-slot velocity demo make it directly testable.
Eight preallocated overlapping voices and independent generator panels support
comparison. Native sound state and parameter automation persist in schema 17.

The five-file upstream target has an explicit single-owner portability mode:
no worker/kit/GUI, C11 atomic dependency or pthread requirement. Original source
hashes, GPL texts, credits and modifications are retained, with app notices and
About attribution. This is an audition subset, not full Geonkick preset/kit
compatibility or the completed eleven-voice Pro-9 instrument.

Official ChowKick 1.2.0 Mac VST3 was extracted without running its installer,
tested for MIDI audio and native embedded UI, and installed in the user VST3
folder. Its original signatures did not verify locally; the local comparison
copy is ad-hoc signed with BSD/provenance notices. No plugin binary is bundled
in Orbit or source archives. See Source/Synth/DRUM_AUDITION_GUIDE.md.

ASan/UBSan core checks cover six recipes, five rates (22.05–192 kHz), exact block
invariance, velocity/tone/drive response, polyphony/extremes, instance isolation
and panic. Instrumented callback paths allocate/free/lock zero times. Strict
C11 compiles with atomics disabled. Production host checks cover MIDI sample
positions/buffer boundaries, real engine renders, native automation, two Geon
panels plus actual ChowKick UI/audio, live Hear hit, real Pattern/Song Play/Stop
window persistence, grouped Undo, Event binding, protected demo slots and
save/reopen. Existing Acid Bass DSP/automation/transport regression also passes.
Listening, hardware performance and actual Windows/Linux builds remain manual.

Build 271 / v0.3.250 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 270 is preserved at Builds/Previous/Orbit_Build_270_before_271.app. The running app was not closed. Successful development iteration 300.

## 2026-10-30 — Build 270 / v0.3.249 Acid Bass internal FX

Collapsible Effects / Drive adds original Soft saturation and Sine Fold, Drive,
Distort blend, Low Cut, Tone and Output. Drag popups replace permanent Acid Bass
value boxes. Existing nine synth parameter IDs and Open303 articulation remain
unchanged; new FX default dry. Schema 16 protects the added native parameters.
Fourfold oversampling uses constant declared latency with aligned dry mixing.

ASan/UBSan checks passed: 44.1/48/96 kHz dry identity/alignment, blend, block
segmentation, finite extremes and distinct shapes; exact native MIDI timing and
panic; native renderer delay compensation, FX and automation; legacy preset
defaults; grouped gestures and right-click Event targets; foldout size/restore;
real production Play/Stop/play-toggle, Pattern/Song, Return, previous/next, Loop
and MIDI/automation Record using hosted buffers. Open and explicitly minimized
windows retained their state and bounds. Hardware and listening comparison are
manual. No exact AudioRealism sonic match is claimed.

Build 270 / v0.3.249 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 269 is preserved at Builds/Previous/Orbit_Build_269_before_270.app. The running app was not closed. Successful development iteration 299.

## 2026-10-30 — Build 269 / v0.3.248 Independent FX and manual returns

FX editors are independent embedded workspace windows, automatically opened after
insertion. Their right-hand controls are minus (hide into the bottom window bar)
and X (remove the owning effect, with Undo). Multiple native and external FX
editors coexist. The bottom bar restores the existing instance and position;
maximized Playlist/Piano Roll leave its FX tabs accessible. Effect windows retain
floating placement and minimized state through unrelated Undo/Redo rebuilds.
Window-menu hide/minimize/cascade actions include FX without deleting them.

External VST instruments now use independent generator windows as well. Opening
another generator leaves the Piano Roll open. Native editors share the proven
Cocoa clipping/scaling wrapper while owning a distinct editor per plugin instance.
Lifecycle callbacks are silenced before window teardown to prevent traversing
dead entries. Valid VST descriptions are registered before instance construction.

New FX returns contain routing only, with no automatically assigned Reverb,
Delay, Chorus or Phaser. Existing saved processors remain intact. Send-knob
right-click menus expose built-in/VST3 choices directly and insert on the shared
return; ordinary send adjustment does not add any processor. EQ/send paint paths
and pointers now use the same angle convention, reaching the same minimum and
maximum endpoints. Audio ranges and project schema remain unchanged.

Validation: ASan/UBSan native FX workspace, Mixer routing/saturation, Peak
Compressor multi-rate DSP/automation and generator controls passed. Real QDelay
DSP/state and two RipplerX instrument instances passed. A separate isolated
native-peer app, with no audio devices, displayed two RipplerX and two QDelay GUIs
simultaneously, checked Cocoa coordinates, retained placement across other panels,
selection and Stop, and minimized/restored FX independently. Its native capture
was reviewed. Real production access-control syntax passed. Listening remains
manual. No public GitHub release was requested.

Build 269 / v0.3.248 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 268 is preserved at Builds/Previous/Orbit_Build_268_before_269.app. The running app was not closed. Successful development iteration 298.

## 2026-10-30 — Build 268 / v0.3.247 Insert and pattern workflow

Mixer inserts now expose one functional left bypass light and a full-width name.
Right-click opens Built-in Effects / VST3 Effects directly. Occupied-slot choices
replace only that effect after construction/validation, preserving neighbors and
one-step Undo. Closing the editor only dismisses its GUI; intentional removal
remains in the context menu.

Playlist wheel/held travel stop at A1/H8 instead of wrapping. Existing-only
navigation handles an unlisted blank current slot. Blocks have a continuous
bottom strip and darker left label; clicking the strip opens all 64 slots across
eight banks, including empty patterns. The resize edge and body gestures retain
their prior roles. Peak Compressor now has eight original metal-style vector
knobs, value entry, needle/readout metering and the retained sidechain chooser.
DSP, automation ranges, serialization and old projects are unchanged.

Native ASan/UBSan Playlist, Mixer, effect-host and Peak Compressor profiles passed,
including slot replacement/Undo, neighbor preservation, stale-target refusal,
single-light interaction, pattern boundaries/selector, fresh reopen, routing,
parameter gestures, actual automation and multirate audio renders. Real production
access-control syntax passed. Headless snapshots were reviewed for the compressor
panel and pattern strips. Device timing and listening remain manual.

KVR research found a reported sine-fold / Zebrify Fold / Reaktor sine-shaper
comparison for the returned ABL2 second distortion mode; it remains a listening
report rather than a verified algorithm. See DISTORTION_RESEARCH.md.

Build 268 / v0.3.247 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 267 is preserved at Builds/Previous/Orbit_Build_267_before_268.app. The running app was not closed. Successful development iteration 297.

## 2026-10-30 — Build 267 / v0.3.246 Original distortion effects

Added native Tube Distortion and Two-Band Distortion under Insert > FX Distortion
and insert choosers. Original nonlinear/filter cores use existing JUCE 4x FIR
oversampling, declared latency and aligned dry blend. Tube includes pre-low-cut,
pre-high-damping, three shapes, Drive/Level/Mix; Two-Band includes complementary
split, independent drives, final tone and Level/Mix. Existing Acid Bass presets
are unchanged; either effect can be inserted on it. Native automation, settings,
chain copy, Undo and schema-15 persistence apply.

Four-rate core sanitizer checks and three-rate native dry PDC/blend, stereo,
folded-harmonic suppression, real output automation, parameter editor, Undo/copy
and same/fresh-process reopen checks pass. The complete automation regression
also checks note-independent Edit-menu curve clipboard routing. DSP is original, not a claimed Orion
or ABL2 clone. MIT candidates and AudioRealism's confirmed ABL2 drive/distortion
return are documented in Source/Mixing/DISTORTION_RESEARCH.md. File menu recheck
confirms entry points; separate stems and explicit sampler production mode remain
open. Pro-9-style synthesis/routing is the next requested generator, not shipped.

Builds 265 and 266 compiled successfully but were superseded before installation
to finish Event Editor Edit-menu routing and retain editor keyboard focus when
using transport/record-mode controls.

Build 267 / v0.3.246 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 264 is preserved at Builds/Previous/Orbit_Build_264_before_267.app. The running app was not closed. Successful development iteration 296.

## 2026-10-29 — Build 264 / v0.3.243 Embedded event and generator workflow

Normal built-in insertion/dock access opens independent workspace controls for
Acid Bass, Drum Rack and 4OSC. Playlist names and Mixer footers double-click to
open generators. Child documents have explicit minimize/maximize/restore/close
controls and stay inside the transport/status boundaries. The Event Editor is
embedded rather than an OS window that falls behind Orbit on transport clicks.
Its toolbar contains four compact tools; commands remain on shortcuts/context
menus. Settings expose fill, four palettes and optional follow. The ruler seeks
without enabling a loop; edge/body drags resize/move it.

A shared LFO Tool previews Song/Pattern curves live, with synced cycles or Hz,
waveform/random hold, amount, offset, phase, fades and curvature. Apply is one
Undo; Cancel leaves source data unchanged. Main transport exposes MIDI/AUTO
recording content; Song Events selects AUTO. Armed automation permits blank
Pattern/Song playback and uses remembered pattern lengths. Generator, automation
(including fresh process), MIDI recording and production syntax checks passed.
Manual device timing and drag/focus listening remain human checks. External VST
instrument workspaces still use the existing shared native editor.

Build 264 / v0.3.243 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 263 is preserved at Builds/Previous/Orbit_Build_263_before_264.app. The running app was not closed. Successful development iteration 295.

## 2026-10-29 — Build 263 / v0.3.242 Original Peak Compressor

Added an original clean stereo peak compressor with Input/Threshold/Ratio/Output,
detector-only EQ, Attack/Release, aligned Dry/Wet, fixed look-ahead, external
sidechain and live input/reduction/output readouts. Host parameter controls honor
range skew and capture one Undo per drag. Native first-sample render checks also
exposed/fixed the engine export writer ignoring frame-view offsets after latency
trimming; a reproducible small dependency overlay is included. Schema 14 protects
the new native type. Existing Compressor state is preserved; CV-82 sonic and
legacy-state equivalence are not claimed. Core and native sanitizer/render checks passed, including real Song automation,
parameter Undo, same/fresh-process state and three-rate transparent PDC.
Build 262 failed on private access in the new Undo wiring; the public Mixer
callback corrects it in Build 263. Hardware listening remains manual.

Build 263 / v0.3.242 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 261 is preserved at Builds/Previous/Orbit_Build_261_before_263.app. The running app was not closed. Successful development iteration 294.

## 2026-10-29 — Build 261 / v0.3.240 Look-ahead limiter

The next Dynamics item now has an original sample-peak limiter with approximately
5 ms look-ahead, linked stereo gain, Input/Release controls and input/reduction
readouts. It registers native latency/tail length, preserves normal loop history
and clears queued audio on time jumps. Core checks cover four sample rates, peak
ceiling, quiet transparency, stereo image, extremes and recovery. Native renders
check parallel PDC, export boundaries, bypass and state/Undo/chain handling.
It is not a true-peak limiter or a claim of Orion DSP equivalence. Schema 13
protects the new native processor from older hosts. Live listening remains manual.

Build 261 / v0.3.240 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 260 is preserved at Builds/Previous/Orbit_Build_260_before_261.app. The running app was not closed. Successful development iteration 293.

## 2026-10-29 — Build 260 / v0.3.239 Independent generator controls

Acid Bass, Drum Rack and built-in 4OSC can open separate control windows through
the generator triangle or Window menu. Each has owner-bound pattern selection and
real instrument controls; Drum Rack note/velocity/sample/pad callbacks use its own
remembered slot. Undo/Redo restores visible controls against replacement plugins.
Close/reopen and saved placement are independent. The shared Piano Roll remains
the main note editor; external-instrument workspace multiplicity remains open.

Build 260 / v0.3.239 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 259 is preserved at Builds/Previous/Orbit_Build_259_before_260.app. The running app was not closed. Successful development iteration 292.

## 2026-10-29 — Build 259 / v0.3.238 Effects hosting and chains

VST3 effect catalogue/browser and targeted insertion are enabled. Slots open native
GUIs or explicit host parameter/event controls. Copy/append/replace chains, unique
IDs, safe sidechain handling, reorder/bypass/remove and one-operation Undo are
implemented. Master Section channels expose four user slots; internal processing
nodes use a separate budget. Render tests cover all eight buses plus Master.
Project VST3 catalogues and exact legacy class-ID discovery remove the global
scan-cache dependency. Installed QDelay passed native rendering, a vendor parameter
round trip, fresh/cache-independent loading and cross-project clipboard checks.
Schema 12 protects larger internal Master graphs. Native look-ahead/detailed Orion
effect DSP, MultiFX containers and vendor GUI/listening checks remain open.

Build 259 / v0.3.238 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 258 is preserved at Builds/Previous/Orbit_Build_258_before_259.app. The running app was not closed. Successful development iteration 291.

## 2026-10-29 — Build 258 / v0.3.237 Sidechains and automation follow

Supported effect parameter windows expose stable-ID sidechain selection with
feedback refusal, native Compressor trigger switching and one Undo. Native renders
verify duck/recovery, source-level response, disconnect and fresh reopen. Generic
host sliders/Mixer controls follow automation without writing edits or fighting
a drag. Master mute now uses a final gate while preserving level curves; native
render tests cover automation silence, later insert order, unmute and legacy mute
migration. Schema 11 prevents older unmute logic retaining the new gate. Particular
VST sidechain layouts and the remaining Orion sidechain effects remain unverified
or unimplemented, respectively.

Build 258 / v0.3.237 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 257 is preserved at Builds/Previous/Orbit_Build_257_before_258.app. The running app was not closed. Successful development iteration 290.

## 2026-10-29 — Build 257 / v0.3.236 Mixer routing and Master Section

Independent Master/bus destination buttons now support parallel outputs with
feedback refusal and one-route Undo. Native graph renders check exact sums,
bus-only isolation, independent sends and a disconnected dry path. A private
output gate avoids the engine empty-output fallback and does not replace its
channel fader. Four factory FX returns are present; unused bus strips hide while
retaining state. New projects use pre-insert Master volume; existing projects
preserve saved order. Changing order migrates both plugin/parameter IDs, native
curves and baselines, including legacy Master pan. EQ soft saturation is original,
optional, bounded and coupled to EQ bypass/settings clipboard. New saves use
schema 10. Exact legacy EQ/send tap response and hardware/multi-output paths
remain open.

Build 257 / v0.3.236 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 256 is preserved at Builds/Previous/Orbit_Build_256_before_257.app. The running app was not closed. Successful development iteration 289.

## 2026-10-29 — Build 256 / v0.3.235 Mixer chapter

Channel and EQ settings copy/paste now retain instruments, insert effects, notes,
event lanes and output destinations. Each paste is one Undo transaction. Channel,
return and bus names can be edited from strip context menus. Send volume linking
uses native pre/post-volume/pan plugin order and survives save/reopen, including
inactive sends created later. EQ and existing sends offer parameter-bound event
menus. Automation and Mixer sanitizer/fresh-process profiles passed. Multiple
destinations, separate send pan taps and EQ saturation remain tracked gaps.

Build 256 / v0.3.235 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 255 is preserved at Builds/Previous/Orbit_Build_255_before_256.app. The running app was not closed. Successful development iteration 288.

## 2026-10-29 — Build 255 / v0.3.234 Automation follow-up

Song Event Editor follows the native cursor and loop, with optional gradient
fill and persisted view preferences. Master/bus/return modules and host volume/pan,
Acid Bass and generic parameter event menus are added. Arranged Pattern curves
repeat with source clips; nonempty Song events override them and Clear restores
Pattern playback. Native automation takes retain fractional timing and commit
all touched lanes with one Undo; first changes on lazy sends/EQ are observed.
Schema 9 preserves fractions and mixer-module lanes. Tempo automation remains
pending, as do EQ/send direct menus and duplicate-instance Pattern identity.
Hardware listening/timing remains manual.

Build 255 / v0.3.234 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 254 is preserved at Builds/Previous/Orbit_Build_254_before_255.app. The running app was not closed. Successful development iteration 287.

## 2026-10-29 — Build 254 / v0.3.233 Song Event Editor

User-requested Song automation usability fix, alongside the active
Playlist/Recording chapter. View > Event Editor now opens a distinct Song
editor for generator/audio-track instrument, insert and volume/pan parameters.
Playlist track Edit Song Events and Playlist Settings open the same editor.
The old unbound overlay toggle is replaced with this explicit entry point.
Track/plugin-slot/parameter selectors, native value labels, horizontal Song bars
and zoom work independently of the selected Pattern. Draw, Line, Erase and whole
graph translation preview locally and commit one Undo on release; Escape cancels.
Curve Copy/Paste, Clear, Smooth and normalized Scale 50%/200% are available, with
Cmd/Ctrl clipboard and Undo keys. View > Hotkeys includes Song editor commands.

Song lanes now carry persisted plugin instance IDs. Duplicate same-name plugins
receive independent native engine curves; ambiguous legacy lanes remain retained
and inactive, while unique legacy targets remain readable. Instance lanes,
including intentionally empty Clear overrides, supersede legacy lanes. Schema 7
prevents older builds from applying instance lanes as name-only automation. Native
Merge remaps imported IDs; Clone Track To maps matching same-name occurrences on
the destination. Project/target changes cancel pending gestures and stale source
commits are rejected. Callbacks resolve IDs after Undo/reload without holding
plugin references past their Edit's lifetime. Engine projection gathers lanes once
per track and preserves unrelated curves, selecting Song or Pattern by transport mode.

ASan/UBSan Song editor, Patterns and Playlist checks passed. Song coverage includes
full normalized drawing range, one-gesture Undo/Redo, cancel/conflict protection,
zero-text Mac clipboard, graph translation/erase, native engine time/value
projection, duplicate-instance isolation, Song/Pattern separation, native save/reopen
and fresh-process reopen, model bounds/NaN handling and import ID remapping. A
production panel snapshot was visually inspected. Known headless CoreMIDI and
existing JUCE string diagnostics remain. Audio listening, physical devices and
external plugin-native input are not newly certified. Live automation recording,
Master/bus/tempo targets, knob-specific entry and arranged Pattern plugin-curve
expansion remain pending; this does not complete the Automation chapter.

Build 253 was superseded before installation after final review added cleanup of stale engine curves for ambiguous legacy targets. The new regression passes under ASan/UBSan. Build 254 / v0.3.233 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 252 is preserved at Builds/Previous/Orbit_Build_252_before_254.app. The running app was not closed. Successful development iteration 286.

## 2026-10-29 — Build 252 / v0.3.231 Mac editing shortcuts and steady Piano Roll view

The macOS JUCE peer deliberately clears textCharacter for Command key events.
Orbit was matching the character alone, so actual Command-C/V/Z and other
advertised shortcuts did not reach their edit commands. EditShortcuts.h now
normalizes letter key codes, covering uppercase native Mac codes and Ctrl
control-character events. The Grid, Piano Roll and root share dispatch. Playlist
Command-A uses the same helper. Text-entry protection stays ahead of host edits;
Option variants and macOS Command-Q/H and Control-Command system chords are not
mapped to pattern tools. The Grid's legacy paste fallback no longer overrides a
rejection from the host dispatcher.

Pattern edit completion captures the first visible pitch row and musical
horizontal offset, then restores them after layout. Paste/cut no longer allow a
delayed new-pattern centering request to shift that view. Same-pattern Undo/Redo
preserve the current view through engine/editor restoration, and restore grid
keyboard focus if the roll owned focus beforehand. A different pattern retains
its normal initial centering. Clipboard data is not transposed by this change.

The full Patterns and Playlist ASan/UBSan profiles and fresh-process checks pass.
New tests use the native Mac key shape (uppercase code, Command flag, zero text)
through actual Grid/window/root methods for C/V/X/A/Z/Shift-Z. They cover blank-slot
paste, exact tick/pitch/velocity and coincident records, source pattern length and
Accent/Slide, Ctrl control characters, repeated edit/Undo/Redo with fractional
pitch scrolling and horizontal scrolling, modifier exclusions and native reopen.
Playlist checks verify zero-text Command-A and protected system chords, retaining
its group/geometry/persistence regressions. Known headless CoreMIDI and existing
JUCE encoding diagnostics remain; the checks do not synthesize OS-level key events
or exercise third-party native editor-owned shortcuts.

Build 252 / v0.3.231 compiled as universal macOS Release (arm64 + x86_64),
was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 251
is preserved at Builds/Previous/Orbit_Build_251_before_252.app. The running app
was not closed. Successful development iteration 285. This is a local usability
fix; Playlist/Recording remains the active manual section.

## 2026-10-29 — Build 251 / v0.3.230 Count-in and audible Pattern overdub

Options > Recording Settings offers 0/1/2/4/8 quarter notes in 4/4; the longer
choices reflect the initial numerator in other meters. The saved option uses
native engine recording preroll and its click range, with no GUI-timer click
scheduler. Count-in sounds when METRO is off; METRO controls clicking afterwards.
A live playback-position sample aligns the existing monotonic MIDI input clock.
Precount notes audition but are not captured, including their later note-offs.
Silent cancellation restores the intended cursor without adding Undo. Record
while already playing begins immediately. An empty Song can start a MIDI take;
its earlier content check incorrectly rejected this despite an active take.

Completed Pattern take notes now replay on later passes through the existing
Generator MIDI processor, before arp/transpose/velocity/channel transformation.
The published sorted replay data is runtime-only. Audio uses a try-lock and a
fixed note-off queue; preparing/swapping/freeing note vectors happens on the
message thread. New publications retain older passes. Boundary-split notes get
their next eligible occurrence separately, preventing immediate recapture/replay.
Stop, commit and panic clear replay; offline rendering excludes it. The document
remains unchanged until one final staged take commit, retaining one Undo/reopen
and conflict retry. Held/sustained notes begin loop replay after their release.

Authored Pattern MIDI clips now repeat through the common multiple of their
active lengths, matching capture and replay. A take requiring more than 512 bars
for that cycle is refused. Starting a take during Pattern playback updates its
projection/cycle once for newly armed silent patterns. This does not implement
Song-loop MIDI recording or independently looping plugin parameter curves.

Recording and Generator ASan/UBSan profiles passed. Actual MIDI-plugin blocks
cover three passes, a mid-take publication, exact on/off times, stop flushing,
unchanged authored data, one Undo/Redo and native reopen. Native ClickGenerator
messages at sample-derived block boundaries verify four precount clicks with
METRO off and continuing clicks when enabled. Deadline exclusion, silent cancel,
status, mixed-length projections and fresh-process preference restoration pass.
Generator checks also verify replay transformation and offline exclusion; the
existing arp/shuffle/settings/routing checks pass. Existing headless CoreMIDI and
JUCE string diagnostics are logged. Physical device timing/latency and the full
stopped-transport preroll handoff still need desktop/audio-hardware testing.

Build 250's first product compile was superseded before installation: a new
non-ASCII Off label used an ASCII JUCE string constructor. Build 251 corrects
that label. Both product invocation numbers are consumed; 250 is not reused.

Build 251 / v0.3.230 compiled as universal macOS Release (arm64 + x86_64),
was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 249
is preserved at Builds/Previous/Orbit_Build_249_before_251.app. The running app
was not closed. Successful development iteration 284. This is a local update.

Playlist/Recording remains
active: fine/free snapping, Song event group policies, pattern-launch recording,
audio capture, Freeze and multitimbral expansion precede Automation completion.

## 2026-10-29 — Build 249 / v0.3.228 Playlist group editing

The active manual section is Playlist/Recording. This increment adds arrangement
rectangle selection, Shift-toggle/add, whole-group move, Ctrl/Cmd-drag copy,
select-all, delete placements and Escape. Selection is held by stable placement
keys rather than recycled per-bar buttons. The complete group is validated before
mutating either the native project or engine. Collision, bounds, frozen/type and
slot-capacity failures cancel the complete operation without a new Undo entry.
A successful gesture has one Undo and keeps the resulting group selected.

Cross-generator transfers clone the complete source pattern into an unused slot,
preserving coincident notes, timing, velocities, custom names, Pattern automation,
Accent/Slide and future children. Repeated placements share one destination slot;
destination slot identities remain intact. Same-generator copies share their
existing source pattern. Audio copies/transfers retain fractional within-bar
position, source offsets, reversal and native clip state, including missing media.
The native setStart call in older audio copy/move paths was also corrected to
move the placement without slipping its source excerpt.

Playlist settings now connect Follow Play Cursor to scrolling, authored note
velocity previews to rendering, and Show Navigation Bar to the horizontal
scrollbar. Choices persist between launches. Focused Shift+S toggles Follow;
hover-N renames the targeted pattern/audio placement with pinned identity.
Audio rename refuses a deleted target and leaves source files unchanged.

The complete Playlist ASan/UBSan profile and fresh-process preferences passed.
Checks cover actual Grid/cell gestures (including macOS Ctrl popup precedence),
atomic group failures and Undo/Redo, full destination bank, extreme integer
bounds, complete pattern cloning, native audio offset/reversal, missing media,
save/reopen, dialog identity after changing active generator, stale audio rename,
follow/disabled-follow behavior, velocity preview pixels and persisted options.
Both navy and light renders were inspected. The harness includes the extra
selection test source before substituting its temporary paths, keeping fixtures
inside isolated validation folders. Known JUCE encoding/headless CoreMIDI debug
diagnostics remain; hardware/audio performance is not certified by this check.

This does not complete the chapter. Fine/free snapping, associated generator and
Mixer Song-event movement, count-in/audible loop overdub, pattern-switch Song
recording, audio input recording, Freeze and multitimbral expansion remain in
Source/Playlist/IMPLEMENTATION.md. Song automation stays at its arrangement times;
Pattern automation copies with the pattern. Automation implementation stays queued.

Build 249 / v0.3.228 compiled as universal macOS Release (arm64 + x86_64),
was ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 248
is preserved at Builds/Previous/Orbit_Build_248_before_249.app. The running app
was not closed. Successful development iteration 283. This update is local.

## 2026-10-29 — Build 248 / v0.3.227 Generator movement and opening latency

Reviewed the 22-second 09-15-07 recording: Ostirus and Labyrinth opening and
Labyrinth movement. Build 247 fixed containment but left drag-time layout and
native coordinate changes in the hot path. Its clip NSView was offset below
the header while JUCE’s NSViewAttachment continued writing peer-relative frames.
Those intermediate writes could jump before Orbit corrected them. The recursive
ProjectView mouse listener could also apply a title gesture again after its
child moved.

Generator movement now changes only the shared surface position and persisted
Piano Roll coordinates; it does not relayout the workspace or repaint all
wallpaper. A maximized roll leaves maximized layout once at gesture start.
Child title/grip callbacks own their gestures, and the parent listener ignores
those forwarded drags. The Cocoa clip host spans the peer at its origin, so
JUCE’s frame writes are valid before any Orbit correction. A stable shape layer
masks the body and foreground panels; its path/hit-test holes rebuild only when
geometry or occlusion changes. Idle timer updates no longer replace layers.

Opening waits for the editor’s real preferred size before fitting, avoiding a
placeholder-size scale/layout pass. Re-selecting the same external plugin and
processor preserves its existing or pending editor; replaced instances still
rebuild. Cold plugin initialization remains plugin-owned and synchronous on the
GUI thread where required. No background-thread editor construction or cache of
all inactive plugin GUIs was introduced.

The full Patterns ASan/UBSan and fresh-process checks passed, including both
title bars, recursive-listener duplicate events, unchanged workspace-layout
count during movement, persisted roll coordinates and prior editing features.
An isolated native desktop ASan/UBSan test passed with installed RipplerX and
Labyrinth VST3s: 60 position changes each, exact peer frames checked before
clipping sync, stable mask/path through 60 idle updates, foreground click
pass-through, toolbar hit testing, smaller-host scaling and inspected captures.
The position-change batches took approximately 1.7/2.0 ms respectively; these
are handler batches, not frame-rate benchmarks. Opening (scan + instance +
editor + fit) measured 68 ms for RipplerX and 6644 ms for Labyrinth in that
run. Cold initialization has not been eliminated. Native tests do not establish
all-vendor/platform compatibility or real-time audio/hardware performance.

Build 248 / v0.3.227 compiled as universal macOS Release (arm64 + x86_64),
was ad-hoc signed/verified, and installed to Builds/Current/Orbit.app. Build
247 is preserved at Builds/Previous/Orbit_Build_247_before_248.app; the running
app was not closed. Successful development iteration 282. This update is local.

The first
two validation attempts exhausted local disk space (compile then link); these
were disposable checks, not product builds. Removed only completed/failed
validation objects and temporary test apps, retaining logs, source, user app
backups and source archives. The harness now keeps line-table debug information
for sanitizer stacks without duplicating full debug type graphs.

## 2026-10-29 — Build 247 / v0.3.226 Generator surface and native toolbar

Reviewed both 08-00-57 and Still issues recordings and the supplied toolbar
proposal ZIP. Its one-content-owner, fixed-toolbar and deferred-editor pattern
is useful; its standalone skeleton was not copied over Orbit’s engine ownership.
The ZIP’s implementation instructions were treated as reference material.

The generator header, pattern controls, active body and audition keyboard now
belong to one opaque GeneratorSurface workspace sibling. Activation raises the
whole surface; routine layout no longer forcibly raises Playlist. Drag and
resize callbacks use workspace coordinates. The Piano Roll’s internal title/grip
now manipulate the host panel directly instead of moving the child and applying
its offset twice. Editor fitting has a reentrancy guard and keeps an open Piano
Roll’s bounds independent of the instrument’s restore size.

External editor creation waits until its wrapper has a visible native peer.
Late editor size changes refit the host; the delayed plugin reference is weak.
Separate effect windows receive the same deferred-size notification. On macOS,
JUCE native plugin views attach directly to the peer outside Component paint
order. A Cocoa clip host now bounds them below the toolbar, applies logical
native bounds for parent scaling, and subtracts front workspace panels from
both painting and hit testing. Cached occlusion survives async size callbacks.
Other platforms retain JUCE’s existing native-view handling. QuartzCore is a
system framework; no new third-party dependency or Orion code is included.

Validation: full Patterns ASan/UBSan and fresh-process checks passed, including
whole-surface parentage, Mixer/Playlist stacking, repeated hosted-title drag
deltas, fixed body origin, GUI/Piano Roll switching, ignored/late editor size
reports and prior velocity regression. An isolated native-peer window loaded
the installed RipplerX and Labyrinth VST3 editors without opening audio devices
or user projects. Inspected actual window captures confirmed the toolbar, full
scaled GUIs, and a correctly positioned foreground mask; native hit testing
passed through that mask to Orbit. The first native capture exposed a vertically
reversed mask and missing Cocoa logical scaling; both were corrected before
product compilation. This establishes those two native editors, not every VST
or Windows/Linux compatibility. The full Generator ASan/UBSan regression also passed (shared Rack dropdown,
collapsed controls, arp/render/Undo, MIDI routing and native project reopen).
Existing headless device/encoding diagnostics were logged separately. Build 247 / v0.3.226 compiled as universal macOS Release (arm64 + x86_64),
was ad-hoc signed and verified, and installed to Builds/Current/Orbit.app.
Build 246 is preserved at Builds/Previous/Orbit_Build_246_before_247.app.
The running app was not closed. Successful development iteration 281. This
update is local; no public release, repository images or earlier archives changed.

## 2026-10-29 — Build 246 / v0.3.225 Per-note velocity lane

Reviewed the 07-59-07 recording. Individual note labels visibly reached 1 and
127, but the lane displayed the arithmetic mean of all notes on a sixteenth.
Thus changing one member of a chord moved the visible bar through only a fraction
of the lane, misleadingly appearing range-limited. The Build 245 endpoint checks
covered authored values but did not check chord bar heights.

Replaced the averaged per-step lane with per-note handles at exact native onsets.
Simultaneous notes get separate horizontal columns ordered by pitch. Each bar
uses its note's full 1–127 height; editing it changes that pitch/onset only.
The active bar and matching note turn gold, with a pitch/velocity readout above
the plot. Note-grid VEL also focuses its corresponding bar. Pattern switches
clear the focus; Undo and native serialization retain the existing note data
model. Exact coincident pitch/onset stacks keep the existing stack-edit rule.
No synth gain or authored velocities are rewritten by the display change.

The full Patterns ASan/UBSan profile and fresh-process preferences passed,
including exact individual 1/127 heights within a three-note chord, separate
handle hit targets, unchanged neighbouring velocities, active pitch/readout,
fractional-onset layout, stroke Undo and both-skin renders. The full Piano Roll
sanitizer/fresh-process profile passed too, including low-MIDI-note lane editing,
scale snapping, mouse-key recording and independent hardware-MIDI routing logic.
Hardware/native display was disabled; existing headless diagnostics remain.
Build 246 / v0.3.225 compiled as universal macOS Release (arm64 + x86_64), was
ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 245 is
preserved in Builds/Previous/Orbit_Build_245_before_246.app; the running app was
not closed. Successful development iteration 280. This update is local.

## 2026-10-29 — Build 245 / v0.3.224 Piano Roll gestures and generator containment

Reviewed all four user recordings (06-23-55, 06-26-20, 06-30-55, 06-34-27) and
the Orion tool-position screenshot. The user corrected the initial removal
request: keep drawing/selection tools and place them at the top. Separate Draw
and Select buttons now occupy the Piano Roll's top left; T toggles them and
Shift-selection remains available in Draw.

New-note/brush entry now floors the floating pointer position to the subdivision
that contains it. Previously nearest-grid rounding moved right-half clicks into
the next cell, including step 1, and rounded integer MouseEvent coordinates could
still cross fine-grid boundaries. Pitch hit-testing matches the rounded painted
row edges at fractional pitch scroll. Move/resize snapping stays separate.
Velocity bars and mouse input share one plot with labelled 1–127 endpoints.
The lane edits directly without VEL mode; individual-note VEL supports unbounded
pointer movement so screen edges do not cap relative dragging. One-gesture Undo
and independent chord velocities are retained.

Added View > Hotkeys with searchable, scrollable implemented commands and focus
context. Added a separate header Restore/Maximize button for Piano Roll, retaining
its floating size without returning to the instrument. Closing still restores
the instrument's own bounds. Floating geometry is saved in GUI preferences.

The VST wrapper previously centred oversized native editors using negative
positions, allowing their macOS native views to cover Orbit's pattern toolbar.
Native editors now anchor at the body origin; fitting measures the actual
post-scale dimensions/transform, with fallback scaling on a plain parent (never
bypassing AudioProcessorEditor's host-scale transform). Late plugin size changes
queue a guarded refit. Header visibility is reapplied by layout for every
selected generator. No Orion binary or proprietary implementation was inspected.

ASan/UBSan full Patterns regression passed, including fractional-scroll row edges,
all cell regions at 8/16/32/64 subdivisions and three zooms, independent note
and lane velocities at 1/127 with Undo, top tool positions, floating/maximized
round trips, Hotkeys search, native-size-based / ignored-scale / late-growing
mock editors, GUI/Piano Roll switching and fresh-process preferences. Both-skin
renders were inspected. The new direct-transform AudioProcessorEditor assertion
found during validation was resolved by moving fallback scaling to a plain parent.
Existing headless String/Component/CoreMIDI diagnostics remain. Hardware was
disabled; actual third-party native views still need the user's desktop re-test.
Build 245 / v0.3.224 compiled as universal macOS Release (arm64 + x86_64), was
ad-hoc signed/verified and installed to Builds/Current/Orbit.app. Build 244 is
preserved in Builds/Previous/Orbit_Build_244_before_245.app. The running app was
not closed. Successful development iteration 279; this update is local.

## 2026-10-29 — Build 244 / v0.3.223 Acid Bass and missing media

Added native Acid Bass using pinned MIT Open303 DSP plus the separately
permissive Ooura FFT, with notices and documented portability/safety patches.
No Orion code or assets were used. The compact original interface has Tune,
Cutoff, Reso, Env Amt, Decay, Accent, Acc Env, Slide and saw/square. Demo pattern
creates an original 16-step phrase without overwriting existing music.
Grid-relative Accent/Slide rows support stroke Undo, native persistence and
whole-pattern copy. Playback supports outgoing slides, same-pitch ties, loop
continuation, live monophonic MIDI/sustain and Panic. This is an acid instrument,
not a claim of bit-identical Orion/303 emulation. MIDI articulation export and
exact arpeggiator/shuffle articulation parity remain outside this pass.

SFS recovery selects whole generators (all selected initially) and optionally
keeps missing dry audio as silent, timed placeholders. Exact known source
metadata and original references survive native save/reopen. File > Find Missing
Media searches a folder recursively and links only unique valid matches; wrong
metadata and ambiguous matches remain unresolved. File > Bypass Missing Effects
disables unavailable native external effect slots while retaining state. It does
not restore unconverted Orion effects, instruments or Drum Rack samples.
Paused SFS binary-mapping research was not resumed.

Help > Quick Test Guide and Source/Editing/TESTING_GUIDE.md give concrete actions
and expected results for the Build 242 tools and the new synth. VEL note labels
show values. Undo now retains the Piano Roll's floating instrument restore size.

ASan/UBSan pure DSP and production host checks passed. Coverage includes native
insertion/editor/demo, subdivision-preserving switches, Undo/copy, sample-timed
stereo rendering, loop ties/Panic, fresh-process parameters/switch persistence,
missing-effect bypass, real Song1 three-track host merge with missing audio,
recursive matching, ambiguity refusal, one-Undo relink and unchanged clip timing.
The SFS UI and full Patterns regression profiles passed. Renders checked both
skins and the compact synth. Hardware was disabled; listening/device latency and
actual external plugin compatibility remain hands-on tests. Existing headless
JUCE debug assertions are logged, not claimed resolved.

Build 244 compiled as universal macOS Release (arm64 + x86_64), was ad-hoc
signed/verified and installed to Builds/Current/Orbit.app with all three DSP
notice files. Build 243 is preserved in
Builds/Previous/Orbit_Build_243_before_244.app. The running app was not closed.
Checkpoint v0.3.223 / successful development iteration 278. This build is local;
no new public GitHub release was published.

## 2026-10-29 — Build 243 / v0.3.222 SFS recovery controls and full Piano Roll

The user screenshots showed a disabled Merge button with no prominent reason.
The SFS dialog now enables Merge when rows are selected, validates on every click
and either sends a fresh plan to the existing host commit or displays the exact
failure above the buttons. Preview remains optional. Changing fields/selection
clears the older report/plan. Host staging failures stay visible and retryable.
The user requested default timing of 96 PPQ / 4/4; both fields now start there
and remain editable. No timing values are inferred from the filename, and the
converter retains its input validation and explicit partial-recovery policies.
Selected Audio rows show required WAV names and the alternative of deselecting
Audio to recover notes only. Instrument/effect state remains unsupported.

The full Piano Roll now uses the same uninset workspace as maximized Playlist,
from below transport to the status bar. App resizing and older saved inset bounds
keep the full coverage; closing restores the instrument bounds. The generator
dock stays behind the full editor so it cannot cover the bottom controls.
Intentional generator movement/resizing leaves maximized mode.

The focused SFS dialog ASan/UBSan check passed defaults, consent, optional preview,
direct merge, invalid/stale inputs, missing media, visible host rejection/retry
and the Song1 notes-only plan (32 notes, 62 placements, two retained unsupported
records). The original fixture remained unchanged. The full Patterns sanitizer
profile and fresh-process preferences passed, with full-workspace geometry and
edge pixels at two sizes, both skins, bottom hit-testing and instrument restore.
Existing headless JUCE assertions remain documented; physical hardware/native
plugin views are not established by these checks.

Build 243 compiled as universal macOS Release, was ad-hoc signed/verified and
installed to Builds/Current/Orbit.app. Build 242 is preserved in
Builds/Previous/Orbit_Build_242_before_243.app; the running app was not closed.
Checkpoint v0.3.222 / development iteration 277. This addresses the current
recovery UI bug without resuming paused SFS format research.


## 2026-10-29 — Build 242 / v0.3.221 Patterns implementation

Returned to the Patterns chapter and implemented its general editing gaps:
independent note-grid VEL/shading, selected-group move/Ctrl-drag duplication,
Alt brush, explicit Rectangle tool, scissor/flips/rotation and focused shortcuts.
Generator-owned Piano Roll settings now cover grid, display, ghosts, follow,
cursor, edit audition and root/scale snapping. Preferences persist in native
engine track metadata. Auditioning edits does not duplicate notes in recording.

Pattern automation gains a cancellable five-wave LFO preview with one-Undo
Apply and efficient staged lane replacement. Switching to a blank pattern clears
the previous native-owned parameter curve, including while stopped. Drawing MIDI
events creates playable automation-only patterns; Pattern audition emits those
events without leaking Song events. Curves span long patterns beyond
the former 4095-step cap, sampled at up to 4096 points with interpolation.
Undo preserves the open Piano Roll and recreates/rebinds the instrument editor.
The user approved postponing the sampler-only RNG ruler until the Sampler section.

The user screenshot also exposed detached GUI/Piano Roll/Rack surfaces. They now
share one content parent with exclusive visibility. Rack uses the common header,
generator settings and pattern/Piano Roll controls. External GUI bodies fit the
reported size beneath a responsive toolbar; the extra INSTRUMENT GUI frame and
outer padding are removed. Built-in 4OSC creates its editor at insertion and
pages its parameter controls for responsiveness. External effects do not replace
an instrument's generator view.

Held vertical Playlist pattern travel now accompanies the wheel, with the same
blank/existing policy, wrapping and one gesture Undo. Horizontal painting and
right-edge resizing remain distinct. The visible Generator-column grip fix is
included. Build 241 passed its checks but failed final universal lipo packaging
when the disk filled; it consumed its number and was not installed. Completed
temporary test executables/objects were removed without removing source, logs,
renders, app backups or source archives. Build 242 compiled successfully as a
universal macOS Release (arm64/x86_64), was ad-hoc signed/verified and installed
to Builds/Current/Orbit.app. Build 240 is preserved in
Builds/Previous/Orbit_Build_240_before_242.app; the running app was not closed.
The checkpoint is v0.3.221 / development iteration 276.

Validation covers the section-specific native host/profile and pure tools, plus
Playlist, Piano Roll, recording and generator regressions under ASan/UBSan,
fresh-process preference restore, long automation save/reopen, both blue themes,
and a 500×500 editor frame without host margins. Physical devices and real
third-party native views remain desktop checks; existing headless JUCE assertions
are not claimed resolved. No SFS work or further manual chapter was undertaken.

## 2026-10-29 — Build 241 / v0.3.220 Generator-column grip

The circled three-line grip in the user's 03:59 screenshot was painted in the
Playlist header, while both resize hit-testing and hover cursor required the
pointer to be below that header. The visible grip and divider now share one hit
region, including in an empty Playlist. Screen-coordinate dragging narrows or
widens the column, shifts the cursor/bar layout immediately, and saves width on
release within the existing 160–480 pixel bounds.

The targeted Playlist ASan/UBSan check passed visible-grip clicks in empty and
populated Playlists, left/right movement and bounds, resize cursor, immediate
cell/playhead alignment, unchanged window/transport/Undo state, release-only
preference writes and restored width in a fresh process. Existing painting,
ruler, drag, native reopen and maximize checks also passed. Hardware/native
desktop interaction remains for hands-on validation; the previously documented
headless JUCE assertions remain, with no sanitizer error reported.

The user also clarified the manual workflow: implement/test each section before
advancing, starting back at Patterns. Playlist/Recording and Automation follow;
Mixer remains unread and on hold. A requirement can be deferred only with the
user's agreement. Existing later chapter reviews remain references.

## 2026-10-29 — Build 240 / v0.3.219 Playlist dragging, cursor and scale snapping

The supplied 03-33-57 recording exposed a header-drag condition that kept testing
the original click against the moving panel's local header. Dragging now latches
the initial hit, tracks screen coordinates until release, and saves bounds once
on release rather than writing preferences for every movement. Ruler clicks
seek without changing loop bounds or enabled state; a deliberate horizontal drag
sets/enables a loop. Stopped cursor display respects initial meter and Song Play
uses the current cursor rather than always returning to zero.

Piano Roll Settings adds optional root/scale snapping with 12 keys and 17 scales,
including double harmonic minor. New drawing, vertical moves and that editor's
clickable keyboard audition/recording use the nearest legal pitch; ties go down.
Off-scale rows are shaded. Existing accidentals stay editable. The user's choice
was Piano Roll only, so hardware and separate virtual/host keyboards retain
their pitch. Application preferences restore the scale after reopening.

Targeted ASan/UBSan host checks passed: upward/reversing drag coordinate changes,
release-only persistence, click/jitter/loop preservation, reverse loop selection,
3/4 cursor placement, earlier Playlist regressions, all scale/root/MIDI-pitch
combinations, real draw/move/Undo and accidental deletion, keyboard capture,
external input independence, both themes and fresh-process preferences.
These checks disable hardware/native display; physical-device and desktop
interaction still need hands-on confirmation. Existing documented headless JUCE
assertions remain; no sanitizer failure was reported.

The ordered manual review now includes Automation after Patterns and Playlist.
Section 6 of ORION_MANUAL_AUDIT.md records the dedicated Song Event Editor,
automation performance capture, stable targets, graph tools and playback gaps.
Next detailed chapter is Mixer. This audit does not implement those additions.
Public tester downloads remain Build 238; this follow-up is a local build.

## 2026-10-29 — manual review through Patterns and Playlist/Recording

Compared System Requirements, Patterns and Playlist/Recording with local Build
239, including the manual's configuration screenshots and shortcut table.
ORION_MANUAL_AUDIT.md sections 3–5 separate completed note-take recording and
Playlist painting fixes from remaining per-note velocity, group editing,
settings, count-in, loop overdub and Song pattern-change capture gaps. The
historical Changes since Orion 7 recording row now reflects subsequent work.
Next manual topic is Automation: Pattern and Song events. This documentation
review does not change application source, product version or build number.
Public downloads remain Build 238; local Build 239 publication and hands-on
hardware/plugin validation remain outstanding. SFS research remains paused.

## 2026-10-29 — Build 239 / v0.3.218 Playlist interaction follow-up

The October 29 screenshots and Orion gesture video exposed a Draw stroke that
created one extended placement rather than repeated copies. It now stamps
native-sized copies with one stroke Undo. Intentional right-edge extensions
remain per-placement overrides; new copies keep the source duration. Resize
and pattern changes refuse overlaps with other placements of the same pattern.

Wheel navigation includes blank A1–H8 slots. Playlist Settings can restrict it
to created slots; the preference survives reopening. Existing remembered-slot
painting is retained. Opaque pattern bodies and inert per-bar hit targets remove
Playlist grid seams while optional note-grid previews remain available.
Maximize now covers the workspace up to the transport/status boundaries, follows
host resizing and restores floating bounds. Targeted ASan/UBSan checks passed
for gestures, lengths, collisions, Undo/Redo, native reopen, both theme renders,
wallpaper coverage and preference restoration in a fresh process.

The public Build 238 release remains available at
https://github.com/tjgoa/orbit-daw/releases/tag/tester-build-238.
Its README update preserved both existing GitHub images. These subsequent
Playlist changes are built and installed locally as Build 239 / v0.3.218. Save,
quit and reopen the Current app to load the new executable.

## 2026-10-29 — Build 238 / v0.3.217 tester checkpoint

The next tester checkpoint adds MIDI recording from clicks and drags on the
left-side Piano Roll keyboard and the host instrument keyboard. They now share
the hardware/virtual MIDI capture path. Pending Pattern notes and held durations
are visible before Stop; Stop commits one undoable take. Without Record, keys
audition normally. Black keys stay shorter than the white-key strip, with
continuous white tails and matching hit-testing in both blue themes and at all
pitch zoom settings.

Options > Wallpapers now opens a local wallpaper folder. PNG/JPEG/GIF files
added there appear when Options is reopened. Choose Image copies into the
library and selects the copy; it preserves originals, reuses identical
same-name images and gives conflicting names a unique suffix. Wallpapers remain
outside the app so replacing the app preserves the library.

This continues Build 237's Pattern/Song MIDI takes, arming, sustain,
quantization, atomic commit and undo/reopen support. Song recording still needs
4/4, looping off and an empty region. Audio recording, controller/automation
performance recording and full Orion compatibility remain unfinished. The
incorrect combined View > Event Editor/Piano Roll entry is logged for the
manual's View/Event Editor pass; automation and note editing are separate.

Native Mac universal, Windows x64 and Ubuntu 24.04 x64 feedback packages all passed CI and are [published on GitHub Releases](https://github.com/tjgoa/orbit-daw/releases/tag/tester-build-238), with current scope, install instructions,
Mac first-launch guidance, source and checksums. No compiler is needed to run a
platform download. macOS builds are ad-hoc signed/not notarized; Windows builds
are unsigned. Hardware audio/MIDI and third-party plugins still need tester
verification. Automated checks cover the mouse recording path, pending preview,
undo, keyboard rendering/hit-testing and wallpaper import/menu refresh in
addition to existing recording and Options regressions.

The exact tester source is published separately from the earlier default-branch
application code. Follow the release tag/source branch for a matching build.
Use copies of projects and report build/version, OS/CPU, audio device,
plug-in names/versions and precise reproduction steps with expected/actual
behavior. Older dated entries below describe their historical checkpoints.

## 2026-10-26 — Build 100 / v0.3.79

The local app at `Builds/Current/Orbit.app` is now Build 100. This pass restarts Tracktion's playback graph immediately after inserting built-in and VST3 instruments, limits Playlist clip controls and drawing to the visible timeline, and replaces the broken bottom-right Playlist glyphs with explicit ASCII labels. The build completed successfully and source archive iteration 155 was created. Runtime keyboard audition and Playlist latency still need a fresh Build 100 launch for confirmation; the open Orbit window remains Build 99.

## 2026-10-26 — Build 94 / v0.3.73

Orbit has moved from a basic DAW shell toward the generator-first workflow that guides the project. This build compiles locally and adds several of the editing behaviors shown in the Orion references.

### Newly implemented

- Add an explicit Pattern/Song transport selector beside BPM. Pattern mode plays all generators’ remembered patterns in sync; Song mode plays the Playlist. Each generator retains its own selected pattern in project state, Playlist playback updates only tracks with clips at the current position, and switching modes preserves the last reached selection.

- Let Play audition the selected, note-filled pattern when the Playlist has no clips; keep arrangement playback when Playlist clips exist. Pattern audition loops until Stop.
- Draw the current transport position in the Piano Roll and Playlist while playing.
- Replace Unicode ellipses in menu strings with ASCII dots to avoid the garbled menu labels shown in the latest screenshot.

- Put Playlist Draw/Move, horizontal and vertical zoom, and grid settings in a persistent lower-right icon strip, matching the familiar Piano Roll control placement.
- Make right-click drag an erase gesture over occupied Playlist cells; empty cells stay empty and do not paint.

- Keep instrument creation focused: inserting a generator no longer opens the Mixer. The Mixer stays embedded in the main workspace, and clicking a generator or Mixer surface brings it forward.
- Make the Piano Roll settings easier to reach with a small keyboard icon and options for grid spacing, emphasis, velocity lane visibility, colored keys, and pitch labels.
- Improve Playlist editing with right-click clip deletion, a pattern menu anchored beside the clicked clip, subtle MIDI note previews, and lighter grid lines.
- Fix Piano Roll note hit-testing so clicking anywhere on a drawn note targets that note for resize or delete instead of creating an overlapping note in an occupied cell. New notes continue using the chosen paint length.
- Keep the velocity lane, keyboard-colouring, and note-name controls visible as direct icon buttons in the Piano Roll footer. Grid subdivision and emphasis remain in the gear menu.
- Continue the VST3 workflow with standard-folder scanning, configurable paths, scan progress, and local diagnostic reports.

### Current focus

Orbit remains in active development. Build 100 compiles successfully and stages the app at `Builds/Current/Orbit.app`. Next priority is confirming immediate keyboard audition and Playlist responsiveness in a fresh session, then checking loop playback and persistence, both automation layers, mixer bus/master routing, Playlist clone/color behavior, and the remaining mixer settings/effects clone options. Plugin-state recovery and broader project compatibility remain later work.

Recent passes added separate pattern/song automation lanes, a dedicated Master Section with four insert slots and no sends, automatic appearance of routed buses, preservation of host automation curves, and the paste-ready Orion native-effects inventory. Build 94 added Playlist generator context actions, pattern/arrangement cloning, and persistent track colors. Build 95 / v0.3.74 persists loop start/length/enabled state and highlights the loop region in the Playlist ruler; loop timing uses the project time signature. Runtime interaction for clone, color, loop wrap/save-reopen, automation and bus routing still needs hands-on verification. Mixer Settings/Effects cloning remains on the roadmap, along with remaining piano-roll, mixer, and Playlist parity work.

See [`BUILD_HISTORY.md`](BUILD_HISTORY.md) for the full build timeline and [`ORION_LEARN_TODO.md`](ORION_LEARN_TODO.md) for the working backlog.

## 2026-10-26 — Playlist workspace and audition follow-up

Build 98 / v0.3.77 embeds the Playlist below the persistent transport toolbar, expands its range to 512 bars, and lets a left-drag on the ruler set the loop start and length. Zoom can show roughly 300 bars in a wide workspace. Selecting Playlist toggles that embedded panel without being tied to Loop. Loop length up to 512 bars is saved with the project.

The generator's duplicate visible resize grip is hidden while keeping its corner resize hit area. A compact two-octave host keyboard appears beneath an open instrument editor and sends note-on/note-off directly to the selected generator, independent of transport state. Build 98 compiles and is installed at `Builds/Current/Orbit.app`. Runtime checks of ruler dragging, zoom range, plugin audition and editor resizing remain pending.
