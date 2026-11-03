# Playlist audio and arrangement editing

Build 323 adds growing waveforms from the native recording thumbnail while audio
is being captured. It does not repeatedly reopen the unfinished recording file.
Committed and frozen clips show their actual source excerpt, reversal and gain.

Right-click an audio clip to Normalize, Reverse, Clone, Make Unique, Split at the
clicked position or open Audio Clip Properties / Trim. Properties expose start,
length, source offset and gain. Normalize measures the played source excerpt in
short message-thread batches and sets non-destructive clip gain to -0.1 dBFS.
It retains the source file, supports Undo/Redo, handles quiet audio above +24 dB
and leaves silence unchanged. It cancels if the clip/recording state changes.

Use Select tool to marquee arrangement clips, or Edit > Select All. Edit > Copy /
Cut, then click a destination and Paste (Cmd/Ctrl+C, X, V). A ruler click retains
the original row layout; a lane click places the first copied lane there. The
clipboard preserves relative timing, native audio references, pattern length
linkage and enabled associated Song-event copies. It currently operates within
the same project. Occupied or invalid destinations cancel the whole paste.

The Playlist Edit menu offers arrangement commands; the Piano Roll Edit menu
keeps its note commands. In Pattern mode, **Add Patterns to Playlist** (F9) places
each unmuted generator's current selected nonempty/event-containing pattern at
the clicked Song bar. **Add Soloed Patterns to Playlist** (Shift+F9) includes
soloed, unmuted generators even when their selected pattern is blank. Audio and
frozen lanes are skipped. It captures each generator's remembered selector,
not the selected Song clips, and keeps Pattern audition running in its mode.

Capture repeats complete patterns to the first common cycle up to 24 bars:
3-, 4- and 8-bar patterns fill a 24-bar block. If the common cycle exceeds 24 bars,
use the longest selected pattern (capped at 24), repeat shorter patterns in full
and leave the remainder empty. Thus 7 and 8 produce an 8-bar block with a one-bar
gap after the 7-bar phrase. A source longer than the cap is explicitly truncated
to 24 bars. Integer full cycles retain source-length linkage. Partial capped
placements retain exact boundaries. LCM multiplication is bounded before it runs.

Each capture advances the blue ruler insertion marker to the shared block end,
so repeated F9/Shift+F9 presses append. Paste also advances to the pasted group's
end. Clicking the ruler/lane chooses a new destination. These operations preserve
running audition time, validate occupied destinations and commit as one Undo each.

## Audio edge trimming and Select slicing (Build 324)

Drag either audio clip edge to trim or restore its source excerpt, including a
long frozen release tail. Edge grips and the resize cursor work with Draw or
Select. Source-file limits and occupied destinations constrain extensions; Shift
temporarily inverts snapping. This does not stretch the audio or erase samples.

With Select, drag across an unselected audio waveform to highlight a time range.
Then drag inside that highlight to detach and move only that excerpt. Audio
before/after it remains in place. A simple click selects the whole clip for normal
movement; Escape clears that selection before marking a different time range.
Marquees from empty space also select partial audio while selecting pattern clips
whole. Cut, Copy, Paste and Delete use the highlighted audio excerpt as well.
Move/copy carries enabled associated Song-event ranges. Source offsets, speed,
reverse state and external fades are retained; new internal cut boundaries have
no inherited fade. The complete operation is one Undo/Redo transaction, including
both remainders, and native Wave state persists normally in the project.

At overview zoom, ruler labels use a power-of-two interval with enough pixel
space for readable numbers. Major guides become quiet vertical lines rather
than complete bright cell rectangles. Close zoom retains the previous boxed
emphasis and note previews. Skin palettes are unchanged by these adaptations.
