# Playlist editing

Build 251 adds quarter-note count-in and audible Pattern loop recording. See
../Recording/README.md for controls, timing behavior and scope.

Build 249 adds arrangement selection and connects the Playlist view options.

- Click **Select** (the arrow beside Draw). Drag a rectangle from empty space to
  select whole clips across tracks. Shift-click toggles one clip; Shift-drag from
  empty space adds a rectangle. Gold borders identify the selection.
- Drag a selected clip to move the group, keeping spacing and lengths. Hold
  **Ctrl** or **Cmd** while dragging to copy. One Undo restores the whole edit.
  Whole-bar placement remains the current model; fine/free snapping is pending.
- Ctrl/Cmd+A selects all placements. Delete/Backspace removes selected placements
  while preserving source patterns. Escape clears the selection.
- Pattern transfers to another generator copy the complete source pattern into
  an unused destination slot, including coincident notes, velocities, names,
  Pattern automation and Accent/Slide. Repeated placements share the copied slot.
- Audio moves/copies retain source offset, fractional position within a bar and
  processing state. Missing-media clips can be edited. Audio requires an audio
  destination; patterns require an instrument destination. An occupied destination,
  full pattern bank, frozen track or out-of-bounds group cancels the complete edit.
- Hover a clip with the Playlist focused and press **N** to rename its pattern
  or audio placement. Audio source files are not renamed. Pattern names apply to
  all placements using that pattern.
- Settings > Follow Play Cursor follows active playback horizontally. **Shift+S**
  toggles it with the Playlist focused. Show Pattern Velocities draws authored
  velocity stems, independently of Show Pattern Notes. Show Navigation Bar toggles
  the horizontal scrollbar. These choices persist between launches.

Song automation is not moved or copied with clips yet. Pattern automation is
preserved. Recording and the remaining chapter work are tracked in
IMPLEMENTATION.md; this increment does not complete Playlist/Recording.

## Earlier pattern gestures

Build 242 adds held-click vertical pattern travel. On an existing pattern body,
hold the left button and move upward/downward to advance/back one slot per 12
logical pixels. Its first significant movement locks vertical selection versus
horizontal painting, preserving normal repeated horizontal strokes. The wheel
and travel gesture use the same blank/existing-slot policy and wrapping. A whole
vertical gesture has one Undo; an intentional right-edge resize remains separate.

Build 241 fixes the Generator-column three-line grip shown in the user's
03:59 screenshot. Its painted header position now uses the same hit region as
the divider below it, with a left/right resize cursor. Drag left/right to set
the column width (160–480 logical pixels), including in a new empty Playlist.
The drag tracks screen movement until release, updates bar/clip/control layout
and cursor alignment immediately, then saves the final width once. It does not
resize/move the Playlist window or activate ruler loop selection.

Implemented against the supplied October 29 screenshots and the Orion
`2026-10-29 03-04-34.mov` gesture reference.

Build 240 also follows the `2026-10-29 03-33-57.mov` report:

- Header dragging latches the initial header hit and follows screen coordinates
  until mouse release, including upward movement and reversals. Moving the panel
  no longer rechecks the mouse-down point against its changing local header.
  Bounds are saved once on release rather than writing preferences every move.
- Clicking the timeline ruler places the stopped/playing cursor at that bar and
  preserves both the loop region and its enabled state. Song Play starts from
  that cursor. A horizontal ruler drag of at least five pixels deliberately sets
  and enables a loop, including a one-bar region or a reverse drag. Ordinary
  click jitter does not enable looping. Cursor placement/rendering respects the
  initial time signature; this does not add a mixed-meter arrangement model.

- Click an empty cell in Draw mode to place the remembered A1–H8 slot. Each
  destination generator uses its own notes for that slot.
- Click an existing clip to select its pattern. Scroll up to advance one slot
  (A1, A2, A3…), including blank slots; scroll down to go back. H8/A1 wrap.
  A selected blank slot becomes a created empty pattern and supports normal
  note editing. Small vertical wheel deltas work; Shift/horizontal scrolling
  stays available for navigation.
- **Playlist Settings > Scroll through existing patterns only** restores the
  created-slots-only behavior. It is off by default and saved in application
  preferences. It is available from the Playlist gear or Options > Playlist
  Settings. Changing it does not edit notes or arrangement data.
- A body drag in Draw mode stamps separate copies at the selected source
  pattern's own duration, rather than creating a stretched clip. Eight-bar
  patterns produce eight-bar copies; a single stroke has one Undo action.
  Reverse strokes also stamp complete copies. The last copy may extend past
  the pointer but never past the 512-bar Playlist boundary. Painting replaces
  whole placements covered by the stroke, preserving their source note data.
- Hover the actual right edge to see the resize cursor, then drag that edge to
  extend the individual placement. This is an intentional arrangement-length
  override; it does not change the source pattern. New copies still use the
  source duration. Normal placements cannot be resized shorter than that
  duration. Imported exact-length recovery placements retain their separate
  model semantics. Existing intentional extensions are preserved.
- A resize or wheel change that would overlap a different placement is refused,
  including another placement of the same pattern. No partial change or Undo
  entry is created. End-of-song painting never truncates a source pattern.
- Pattern bodies are opaque and per-bar hit targets do not paint hover/focus
  borders. Playlist bar lines stay in empty cells. The optional miniature
  Piano Roll grid inside a pattern is an independent preview setting.
- Maximize fills the complete workspace between the transport and status bar,
  with no outer wallpaper margin. It follows host resizing and restores the
  previous floating bounds.

Run `python3 Tools/test-options-host.py --sanitize --playlist` after a normal
CMake build. This exercises actual cell mouse/wheel handlers, native placement,
Undo/Redo and save/reopen, both themes, maximize/restore and wallpaper coverage.
It also reproduces the changing mouse-down coordinates during upward header
movement, verifies release-only persistence, and checks ruler clicks, jitter,
loop preservation, reverse range selection and 3/4 cursor placement.
The visible column grip is tested with no generators and with a populated
Playlist, in both directions and at its bounds, including fresh-process width
restoration and unchanged transport/project Undo state.
The preference is verified in a fresh process. Audio hardware and native desktop
interaction are disabled in the harness; the broader app retains the previously
documented JUCE encoding/headless CoreMIDI debug assertions.
