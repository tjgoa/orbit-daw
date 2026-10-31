# Generator options

The downward-triangle button at the left of the generator header opens the
hierarchy shown in the supplied
`2026-10-29 00-39-11.mov`: Rename Generator, Generator Settings, Rename Current
Pattern, Arpeggiator/Shuffle, Link To Next MIDI Controller, and Receive MIDI From
Generator. It remains available when pattern controls are hidden or the generator
is rolled up. The separate Drum Rack window has the same dropdown in its own
top-left header, targeting that rack even when another generator is selected.
Dialogs keep the original generator ID and pattern slot when selection
changes. Opening or cancelling a dialog does not change notes or add a processor.

The arpeggiator follows the behaviors described in Orion's installed Help chapter
Additional Features > Arpeggiator and Shuffle, read through the running Orion app.
The installed manual is compiled HTML (`Help/Orion Help.chm`), rather than a PDF.
No Orion source code, manual text/images, proprietary arpeggio files, or internal
parameter tables are included in Orbit source or its archives.

## Arpeggiator and Shuffle

- On enables generator-wide arpeggiation before the instrument, for sequenced
  patterns and live MIDI. Disabling it restores direct MIDI playback.
- Reset on Chord Trigger restarts the sequence when a new chord follows silence.
  With it disabled, the sequence position continues across gaps. Panic clears
  sounding notes, sustained keys and queued events.
- Note Order provides Up, Down, Up/Down, Alternate Up (endpoint repetition),
  Alternate Down, and a user sequence of semitone offsets from the lowest note.
  These are Orbit's original implementations; the less-documented alternate
  traversals are not claimed to be identical to Orion's implementation.
- Range spans 1-5 registers. Distance sets the semitone interval between them.
  Out-of-MIDI-range generated notes are omitted, never clamped to another pitch.
- Gate changes duration relative to the chosen rate: Off/0 means one interval.
  Retrigger limits the note cycle; Repeats gives a finite cycle count or unlimited.
  The 16 rhythm buttons toggle steps between notes and rests.
- Tempo supports whole through 64th notes plus 8th/16th/32nd triplets. Shuffle
  delays alternate cells at its independently selected rate and works with the
  arpeggiator disabled. Direct-note shuffle delays matching note-offs as well,
  retaining their durations. Live shuffle uses the running processor clock;
  transport playback aligns that clock with the edit's musical position.
- Load MIDI imports 1-32 note pitches as a user order. It is a standard MIDI
  workflow, not a parser for Orion's undocumented arpeggio preset format. Files
  over 1 MiB and offsets beyond 48 semitones are refused without applying changes.
- Apply keeps the authored notes unchanged. The original processor runs in the
  audio graph before 4OSC, Drum Rack or the hosted instrument. It is excluded from
  the Mixer's audio insert slots. Audio rendering includes this processor.
- Render writes arp/shuffle results into the pinned current pattern for its
  existing cycle length, then disables generator-wide arp/shuffle to avoid double
  processing. One Undo restores notes and settings together. General generator
  transpose/velocity/channel controls remain playback settings. An all-rest rhythm
  intentionally renders silence. Rendering refuses patterns over 65,536 steps
  or output beyond 262,144 notes before changing the project.

Generator Settings exposes input channel, output channel, playback transpose and
velocity scale. Input channel uses the existing Advanced MIDI routing mode.
Output, transpose and velocity apply to sequenced and live MIDI. Authored notes
and the Piano Roll remain unchanged. These are host controls; synth-specific
polyphony, glide or tuning controls remain in the instrument's own editor.

## MIDI linking and receiving

Link To Next MIDI Controller offers generator volume, pan and the actual
instrument parameters. The next CC on an enabled input stores a device/channel/CC
binding to that generator's stable ID. Later generator selection does not retarget
it. Assignments use the existing user controller preferences; subsequent changes
to generator parameters participate in project undo. Cancel Controller Link
disarms learning. Deleted/missing generator targets are ignored.

Receive MIDI From Generator configures the engine's native track MIDI input.
The route is saved in engine state, with a project-generator-ID property for menu
checks and cycle detection. Feedback loops are refused before changes; other
connection failures restore the prior project/session snapshot. None disconnects
the source. Connection/disconnection are undoable. The current engine provides
one destination per track MIDI source; a source already feeding another generator
is refused instead of silently displacing that route.

This route receives the source track's MIDI output. Plugins that consume incoming
MIDI without forwarding/outputting it may therefore provide no MIDI to receive.
Track MIDI routing is an engine live/transport path; its graph builder excludes
track-device sends during offline audio rendering. Offline rendering of these
inter-generator routes remains a separate engine-integration task. Ordinary
generator arpeggiation/shuffle itself is processed during offline rendering.
MIDI file export continues to export authored pattern notes; use Render first to
export baked arp/shuffle notes.

## Verification

With an existing Orbit CMake build:

```sh
python3 Tools/test-options-host.py --sanitize --generator
```

The harness uses production generator/menu/settings/processing code and actual
engine plugin instances with isolated preferences. It substitutes dialog display,
native window centering and hardware initialization. Checks cover musical output,
audio-block MIDI timing/panic, non-destructive Apply, Render/Undo, general settings,
names, identity-pinned controller learning, native MIDI routing and feedback
refusal, save/reopen and panel images. Regression checks also exercise the visible
header button with hidden pattern controls/rolled-up generators, and open the
Drum Rack menu while another generator is selected before rendering its remembered
pattern. Existing encoding and headless device debug
assertions are logged separately; this is not a hardware or native UI certification.
See `ORION_MANUAL_AUDIT.md` for the reference review and remaining work.

Build 235 desktop check: the drawn header triangle was inspected in an isolated
GUI harness with separate settings and hardware initialization disabled. Native
popup opening was not confirmed: automated clicks on both the dropdown and
unrelated test-window buttons left the view unchanged. The isolated window was
closed; the user's running app/project was not closed. Restart the Current app
after saving to load Build 235, then choose the top-left triangle >
Arpeggiator/Shuffle to test audible playback.
# Attached generator editor — Build 242

The instrument viewport, Piano Roll and Drum Rack are children of one generator
content area. Only the active view is shown. Moving/resizing the generator keeps
its content attached; opening Piano Roll hides the native GUI and Rack first.
The Rack uses the shared header, settings dropdown and pattern/Piano Roll
controls. Its old separate floating header no longer covers the note editor.

External instruments fit their reported editor size directly below the common
header/toolbar, with a responsive toolbar for smaller editors. The extra
INSTRUMENT GUI heading and left/right/bottom host frame are removed. Fixed-size
editor aspect ratio is retained during scaling/resizing. An existing separate
host editor is closed before embedding that instance's editor. External effects
do not replace a built-in instrument's generator GUI.

The built-in 4OSC now creates its parameter editor at insertion. Its controls
are paged in groups of 64 to keep creation/restoration responsive while exposing
all reported parameters. Undo recreates/rebinds editors to the restored session
and preserves an open Piano Roll. Headless tests verify parentage, exclusive
visibility, shared Rack controls and an exact 500×500 content frame; native
third-party view behavior remains a desktop compatibility check.


## Shared generator surface — Build 247

`GeneratorSurface.h` is the single workspace sibling for the header, pattern
controls, body and audition keyboard. All bounds inside it are local. Drag
labels convert pointer coordinates to the workspace; Piano Roll’s internal
title and grip send cumulative gesture deltas to the same host callbacks. They
never reposition the child editor. Raising or laying out a panel must not split
the generator’s header/body stacking or implicitly raise Playlist.

External editor creation waits for a visible peer. Preferred-size callbacks
refit the host under a reentrancy guard. Plugin references used for deferred
creation are weak; editors do not own their engine plugin. The existing plugin
instance remains owned by the engine and the editor is destroyed before it.

On macOS, `NativeViewClip.mm` reparents JUCE’s NSView hosting surface into a
clipping NSView under the same peer. It preserves the untransformed logical
bounds for native scaling, clips below the header, and masks foreground workspace
rectangles from rendering and hit testing. The native view is restored when the
clip is cleared; plugin instances/editor contents are not transferred. Clip
updates disable implicit layer animations. Occlusion updates on workspace layout,
activation and the UI timer. A macOS-specific source/framework registration lives
in CMake; Windows/Linux use JUCE’s existing implementation.

Native smoke test (only the explicitly supplied plugins are loaded):

```sh
python3 Tools/test-options-host.py --sanitize --native-plugins \
  /Library/Audio/Plug-Ins/VST3/RipplerX.vst3 \
  "$HOME/Library/Audio/Plug-Ins/VST3/Labyrinth.vst3"
```

This isolated test needs an interactive macOS desktop session. It opens its own
window, captures that window, tests native hit pass-through, then exits; it does
not initialize audio/MIDI devices or change Orbit preferences. The app sandbox
may need permission to launch this test window. Real RipplerX/Labyrinth captures
were inspected for full GUI scaling, toolbar separation and foreground masking.
Remaining compatibility checks include other vendors, plugin-internal popups,
plugin-specific resizing, mixed display scales and Windows/Linux native windows.

## Position-only movement — Build 248

A drag updates the GeneratorSurface position, mirrored persisted roll position,
and native clipping. Do not call the full ProjectView::resized/repaint path on
each pointer event. Dedicated child callbacks handle title gestures; the recursive
parent mouse listener must not apply their events again after the child moves.
Leaving maximized mode may perform one initial layout.

The native clip NSView spans the peer at (0,0), with a body-shaped layer mask.
JUCE writes peer-relative native frames even after reparenting. An offset clip
parent makes those intermediate writes incorrect and causes jitter. Keep the
clip in the peer’s coordinate system; update the body mask/hit area instead.
Reuse its shape layer/path while geometry and foreground rectangles are unchanged.

Native validation checks each frame immediately after Component movement, before
clipping sync can repair it; exercises 60 moves, idle mask stability, foreground
hit pass-through and a smaller-host scale change. Timing output reports plugin
initialization/editor setup separately from the position-change batch. Cold
plugin load time still varies by vendor and patch/sample resources.


## Independent built-in controls — Build 260

Use the generator triangle > **Open Separate Generator Controls**, or the same
entry in **Window**, for Acid Bass, Drum Rack and built-in 4OSC. Repeat for another
generator to leave both control windows visible. Each owns its actual instrument
editor and A–H / 1–8 selector. Drum steps, velocity, sample loading and pad settings
resolve the window's generator and remembered pattern, regardless of the active
main editor. The detached drum display follows edits made in the shared editor
when no mouse gesture is held. Clicking a control window makes that generator
active; its Piano Roll button opens the shared Piano Roll for that owner.

Close hides only that control window. Reopening reuses it; bounds are saved per
generator on close/destruction rather than on every movement frame. Undo/Redo
recreates visible windows against the restored native Edit and preserves bounds.
New/open project and generator deletion clear old windows before destroying their
plugin instances. Hide All, Minimize and Cascade include these control windows.
Expanding a drum sample editor resizes its host window while keeping the body
attached; collapsing restores its prior height.
Command/Ctrl editing shortcuts are routed asynchronously to the owning generator
so an Undo cannot destroy a window inside its own key callback.

This increment provides separate built-in controls. Multiple independent Piano
Rolls and simultaneous external-instrument bodies in the workspace remain open;
the existing embedded generator/piano-roll surface is still the main editor.
No project-schema change is needed for application window preferences.

Validation: `python3 Tools/test-options-host.py --sanitize --generator-controls`
checks actual Acid Bass/Drum Rack controls, generator/pattern isolation, grouped
Undo/Redo with window restoration, close/reopen, placement, sample/parameter/note
state and Edit lifetime. The Patterns regression covers shared editing/viewport
behavior. Layouts are rendered offscreen when no desktop is available. Physical-device,
native window focus and multi-display interaction still need manual tests.


Build 264 makes independent built-in controls the normal insertion/dock path,
inside the Orbit workspace, with per-panel minimize/restore/close and safe title
movement. Playlist names and Mixer generator footers double-click to open them.
Shared Piano Roll remains the note editor; external VST GUIs retain their current
shared editor until the multiple-native-editor pass. Old desktop-coordinate
control placement settings are replaced by workspace-coordinate settings.
