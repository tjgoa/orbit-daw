# Song Event Editor

Open **View > Event Editor**, right-click a Playlist track and choose
**Edit Song Events**, or use **Playlist Settings > Open Song Event Editor**.
Choose a track, then a plugin/parameter. Plugin slots distinguish repeated
instances. The value ruler uses the parameter's own formatting and range.

The graph follows song bars, independently of the selected pattern. The initial
view covers the Playlist's 512 bars; horizontal scrolling and 50/100/200% zoom
allow detailed work. Existing later Song points extend the editable range.
Draw interpolates between pointer samples; Line previews a straight segment;
Erase removes points across a dragged interval; Move Graph translates the whole
lane horizontally without truncating its points. Mouse-up commits one Undo edit.
Escape cancels the pending gesture.

Copy/Paste transfers the complete normalized curve, preserving song positions,
between parameters/tracks. Clear removes all points for the target. Smooth uses
a 1:2:1 neighbor average with fixed endpoints. Right-click offers Scale 50%/200%
of normalized values, clamped to 0–1. Cmd/Ctrl-C/V/Z/Shift-Z and Undo/Redo buttons
operate on these Song edits while the graph has focus. Clipboard data is internal
to the open editor; it is separate from the note clipboard.

Choose **Song** in the main transport and Play an arranged pattern/audio clip
to hear the curve. The red cursor uses the native transport clock; the gold loop
range follows the main Song loop, including its enabled state. **Follow** scrolls
with playback and returns on a loop wrap. **Gradient fill** shades the curve.
Both view preferences survive restart and do not create project Undo entries.

The module selector also offers Master, existing output buses and FX returns.
Right-click a host volume/pan fader, Acid Bass knob or generic parameter slider
for **Edit Song Events** or **Edit Pattern Events**. External plugin GUI menus
remain plugin-owned. Build 256 also adds EQ/send event menus; adjust an inactive send first to create
its target. All those parameters remain available in the Song selector.

Pattern plugin curves now repeat with their arranged clips, including extended
placements. Gaps restore the captured baseline. Nonempty Song events override
Pattern automation for that parameter across the song. Clearing Song events
restores arranged Pattern playback; the empty instance lane still suppresses
older name-only Song data. Duplicate-name Pattern targets remain unsupported by
that older lane identity; their Song lanes retain exact plugin-instance IDs.

Use **Record** in the Song Event Editor to record knob movements in Song mode.
Alternatively choose **Options > Recording Settings > Automation**, then Record
in the main toolbar. Pattern mode writes each generator's selected Pattern;
Master/bus and ambiguous Pattern targets use exact-instance Song lanes. Capture
starts immediately at the current cursor (MIDI count-in does not apply). Stop or
toggle Record to commit all touched parameters as one Undo transaction. Untouched
parameters are preserved. The native engine recorder supplies timestamps and
punch-out curves; recording is not sampled by the GUI timer. On repeated passes,
later writes replace the overlapping native curve region. Pattern coordinates
fold into the selected pattern length; the native loop-end reset is excluded.
Actual device timing and listening still need a hardware test.

Schema 9 preserves fractional sixteenth-step automation positions and global
mixer-module lanes. It loads older supported files; new saves require Build 255
or newer. Native Merge remaps generator plugin IDs. Clone Track To maps existing
same-name occurrences. Legacy name-only Song lanes remain readable where unique.

Song tempo automation remains a separate timing/playback/export task. It is not
part of this checkpoint. Simultaneous generator windows are separately tracked
in ORION_LEARN_TODO.md.

Validation: `python3 Tools/test-options-host.py --sanitize --automation` exercises
production handlers and engine curve projection with isolated preferences and
hardware disabled; fresh-process reopen follows the main test. Patterns and
Playlist profiles provide regression coverage. Listening and desktop input
checks with particular external plugins remain manual.
