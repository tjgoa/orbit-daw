# Options menu

Implemented from the user-supplied `Movies/OPTIONS MENU.mov` reference, using
Orbit-owned appearance and the available JUCE/Tracktion device APIs. All reference
menu entries are enabled and have handlers; device/project-dependent controls
report unavailable targets rather than pretending to apply a setting.

| Entry | Behavior |
| --- | --- |
| Skins | Navy Blue switches Playlist/Piano Roll to the existing dark blue canvases and Mixer to Midnight Navy; Light Blue switches both canvases to their existing cool light blue/gray look and Mixer to Classic Blue. Both presets persist together. Default, Azure, Classic, Grey and Mixer palettes remain available. Hosted instrument artwork remains owned by its plug-in. |
| Wallpapers | None, original Slate Gradient or Blueprint Grid, or a user-selected PNG/JPEG/GIF. Fit, Fill and Tile placements persist. Custom images are referenced by path and are not embedded in projects or source archives. |
| Audio Input Settings | Device/channel selector, actual input-channel enable/disable, mono/stereo engine inputs, monitoring, 16/24/32-bit recording format, sample-based input compensation, and explicit assignment to the selected audio track. Monitoring is off by default. |
| Audio Output Settings | Device, output channels, supported sample rates and buffer sizes; project CPU-core policy, global CPU protection threshold and output clipping control. |
| MIDI In Settings | Device enablement, Easy/Advanced routing, generator input channel, pitch-bend/aftertouch/program-change/transport filters, and pedal enablement/polarity. |
| MIDI In Controllers | Multiple saved CC assignments, input-device and channel filters, Learn Next CC, selectable instrument/mixer/transport targets, independent minimum/maximum ranges and assignment removal. |
| Enable MIDI Out | Toggles the selected MIDI output. Opens its settings if no output has been selected. Disabling releases external notes. |
| MIDI Out Settings | Output-device selection, MIDI clock/start/stop, timing adjustment and optional generator routing to MIDI instead of audio. Live input and the virtual keyboard also support MIDI thru. |
| Virtual MIDI Keyboard | F12 toggles a separate polyphonic keyboard with mouse/computer-key input, MIDI channel, velocity, octave and All Notes Off. Closing releases its notes. |
| Sample Paths | Existing folder-list editor with Add/Remove/Save/Cancel. Audio import and Drum Rack sample loading start in the first existing configured folder. |
| VST Paths | Existing folder-list editor, standard VST3 locations and optional startup scan; extra folders feed the actual VST3 scanner. |
| Time Signature | Initial numerator/denominator applied to project and engine, plus persisted triplet piano-roll subdivision policy. |
| Timebase | MIDI export PPQ (24–15360) and transport display: bars, seconds, or bars:beats:ticks. Existing native note timing is retained. |

The top menu follows the reference order, starting with Skins and Wallpapers;
Navy Blue and Light Blue are first in the Skins submenu. Selecting a preset updates
all existing surfaces immediately and saves the choices for later reopening. Local
surface overrides remain available and clear the preset checkmark until reapplied.

Orbit's additional Loop, Metronome, Diagnostics, Preferences, Project Settings and
Playlist Settings entries retain their existing working handlers. Preferences
remain separate from project settings. The device window is placed on the modal
stack when opened from an Options form, so its controls can receive input.
Audio device changes take effect immediately in that window; cancelling the parent
form does not undo an already-applied hardware device change.

## MIDI behavior

Easy mode routes notes to the current instrument generator. Advanced mode routes
to all non-audio generators whose saved channel matches, including Omni targets.
Note-offs retain the original recipients when generator selection changes.
Identical pitches held on different input devices are tracked independently.
Pedal release and panic reach previous recipients too. Project replacement, undo,
input-policy changes and shutdown release tracked notes.

A bounded asynchronous inbox delivers hardware events on the message thread.
Overflow clears held notes before accepting the retained events. System-exclusive,
active-sensing and incoming MIDI-clock messages are ignored. The transport filter
supports Start, Continue, Stop and Song Position, not external tempo/clock following.
Physical engine MIDI input devices are disabled to avoid duplicate unfiltered
input alongside Orbit's routing path. MIDI take recording is described in `../Recording/README.md`.

Controller assignments address selected-track volume/pan, master volume, tempo,
play/stop, or a parameter on the selected instrument. Parameter CCs use that
parameter's normalized range. Transport buttons trigger on a rising edge. A run of
CC changes on the same target shares an undo snapshot; unchanged values create no
additional snapshot. Learn fills the fields; Apply saves the assignment. Missing
instrument parameters are retained in preferences and skipped when unavailable.

MIDI output routing uses the engine's normal track output. It sends MIDI leaving
the generator's plug-in chain; plug-ins that consume MIDI may not pass it onward.
It is not a separate pre-instrument MIDI split. Device enable/disable can recreate
SDK device objects, so policy code re-finds outputs by ID and never uses a pointer
after enabling/disabling it. Native MIDI output timing/clock and hardware thru
still need physical-device validation.

## Persistence and timing

Appearance, hardware/MIDI policies and controller assignments use application
preferences. Generator input channels use `orbitMidiChannel` in engine track
state. The optional engine properties `orbitMidiExportPpq`, `orbitTimeDisplay`
and `orbitTripletGrid` persist timebase/meter UI policy and participate in project
undo and native save/reopen. No native schema or existing note tick scale changes.

MIDI export rounds note positions to the chosen file PPQ and ensures matched
note-offs remain at least one output tick after their note-ons. Reducing file PPQ
can lose export timing precision; the native pattern remains unchanged. Native
pattern cycles and Playlist placements retain Orbit's current step/bar model;
this does not implement general mixed-meter arrangement editing.

Record now captures MIDI takes; audio recording remains unfinished. Input format,
monitoring/routing and audio recording compensation configure the engine but do
not complete audio recording. Orion-specific driver controls are replaced by available engine/device
controls; this work does not add a double-precision mixing engine.

## Verification

Build with the project's numbered build process, then run:

```sh
python3 Tools/test-options-host.py --sanitize
```

The check runner uses the actual product compile flags and linked CMake objects.
It creates isolated settings, cache, logs, test source and executables in a unique
temporary folder. It bypasses native display centering/modal presentation, replaces
the app entry point, and disables audio-hardware initialization. Production menu
builders, panel fields, policy/routing handlers, note data, undo and native saving
run unchanged. F12 window presentation is captured; keyboard state and polyphony
are tested separately using the actual keyboard content.

Checks cover menu IDs and absence of placeholders, real panel control wiring and
renders, routing/filter/pedal behavior, recipient-safe note-offs, overlapping-device
notes, panic, CC learning and normalized parameter mapping, grouped/no-op undo,
MIDI-output recreation/thru/panic with a simulated SDK device, export PPQ and
nonzero note lengths, unchanged native timing, native save/reopen, appearance
settings, sample-folder selection and bounded asynchronous delivery. Coordinated
blue presets are checked across all three real surfaces, rendered separately, and
verified after fresh process startups using the saved settings.

Physical audio/MIDI devices and native desktop interactions are not verified by
this harness. Existing JUCE text-encoding debug assertions elsewhere in the host
remain, and MIDI enumeration can report a CoreMIDI-client assertion in the
headless environment; this is not an assertion-clean whole-app run. The installed Current app
and older source checkpoints are preserved. The SFS importer goal stays paused.
