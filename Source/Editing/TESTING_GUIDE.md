# Orbit: practical testing guide

Builds 242–267 retain these editing features. Start with a new test project.
For chord tests use **Insert > Synths > Built-in 4OSC Synth**. Acid Bass plays one
note at a time; use it for the Accent/Slide test at the end instead.

## Build 270: Acid Bass Drive / Distort

1. Insert **Acid Bass**, click **Demo pattern**, and play in Pattern mode.
   Open its generator controls and click **Effects / Drive +**. The five FX
   knobs appear below the synth. Click again to fold them away.
2. Turn **Distort** from 0% toward 100%, then turn **Drive** up. Switch
   **Soft** to **Sine Fold**: the latter should become more metallic/folded at
   stronger Drive settings. Compare at a comfortable level using **Output**.
3. Sweep **Low Cut** to remove bass from the distorted portion and **Tone** to
   darken it. Distort 0% restores the dry synth (Output still controls level).
4. Values appear while dragging knobs; permanent value boxes are gone. A single
   drag should undo in one Command-Z. Right-click an FX knob to open its Events
   and draw/record automation. Save/reopen: the audio settings should remain.
5. Keep Acid Bass, Drum Rack, Playlist, Piano Roll, Event Editor and two FX panels
   open. Minimize just one FX. Press Play, Stop, Play again to stop, Return,
   previous/next bar, Loop and Record, in Pattern and Song mode. All panels should
   retain their own positions and open/minimized states. Stop should not dismiss
   the Event Editor or a plugin. Fold/unfold Acid FX without moving other panels.

These effects are an original ABL-inspired design; a precise AudioRealism sound
match has not been established. Save a backup before opening a schema-16 project
in an earlier build (earlier builds reject the newer format).

## Build 269: independent FX windows and manual send effects

1. Create a new project. Right-click an insert and choose Peak Compressor: its
   panel opens immediately. Add Tube Distortion on another channel. Both panels
   should remain open, with their own positions, alongside Playlist, Piano Roll,
   Event Editor and generator panels.
2. FX titles now have exactly **minus** and **X**, on the right. Minus hides the
   panel into its bottom-bar tab. Click the tab to restore the same plugin and
   window position. X removes that effect; Command-Z restores its saved state.
   The insert's left light remains the bypass control.
3. Open another generator, change Mixer/Master view, or press Play/Stop. These
   actions must leave unrelated FX panels open. Undo of another edit preserves
   existing FX windows' floating positions and minimized states.
4. Insert two external VST instruments (e.g. two RipplerX instances). Their own
   generator panels should remain open side by side, along with built-ins.
   Selecting a different generator must leave the Piano Roll open.
5. New FX returns start empty. Right-click any of the four send knobs: Built-in
   Effects and VST3 Effects appear immediately. Choose an effect to insert on that
   shared return. Raise the send to hear it; choosing an effect does not silently
   change your send level. Canceling the menu changes nothing.
6. Master/return strips retain their manual insert slots. Saved projects keep
   their existing effects; no processors are stripped from old sessions.
7. EQ and send knobs: compare minimum, midpoint and maximum. The blue arc and
   pointer use the same sweep, ending at the lower-right endpoint at maximum.
   The intentional gap is centered at the bottom. Audio ranges are unchanged.
8. Maximize Playlist/Piano Roll with FX windows present. The bottom FX tabs remain
   accessible below the document area. Restore an FX and check that the other
   panels retain their positions.

## Build 268: insert workflow, pattern selector and Peak Compressor panel

1. Mixer: right-click an empty insert. Built-in Effects and VST3 Effects should
   appear immediately. Choose Peak Compressor. The row should contain one light
   on the left and its name, without a second bypass button or inline X.
2. Click the left light: it bypasses/enables the effect. Click the name: it opens
   the panel. Close the panel using its X: sound processing stays active. Remove
   Effect remains available deliberately in the slot's right-click menu.
3. Right-click an occupied slot and choose another effect. It replaces that slot
   while keeping the effects before/after it. Command-Z restores the old effect.
4. Peak Compressor: try Input, Threshold, Ratio, Attack/Release and Dry/Wet. The
   rotary controls retain their previous ranges and text entry. Reduction meter
   should react while input/output meters follow audio. Try an automation curve
   and sidechain source; close/reopen the project to check saved values.
5. Playlist: wheel or hold-and-drag vertically over a pattern's BODY. Downward
   changes stop at A1; upward changes stop at H8. Repeat with "existing patterns
   only" enabled: the first/last eligible pattern should also stop.
6. The pattern block has a continuous bottom strip and darker left label. Click
   that strip for the eight bank submenus containing all 64 slots, including
   empty ones. Pick blank A4, draw notes there, then return to your old slot.
   Body drags still paint/move; the right edge still resizes.
7. The previously added Tube Distortion and Two-Band Distortion are under Insert
   > FX Distortion and the insert choices. On an Acid Bass demo pattern, try Tube
   Drive/Shape/Hi Damp, then Two-Band Breakpoint/Low Drive/High Drive. Lower Level
   to compare at similar volume, and test Mix and the single insert bypass light.
   They are original effects; an ABL2 sine-fold equivalent is still research.

## Build 267: Tube and Two-Band Distortion

1. Select Acid Bass (or another generator), then **Insert > FX Distortion > Tube
   Distortion**. Start quietly, increase Drive, compare Symmetric/Tube A/Tube B,
   and use Level to compare loudness. Lo Cut/Hi Damp change what reaches the drive.
2. Add **Two-Band Distortion** instead. At Breakpoint 800 Hz compare Low Drive
   versus High Drive, then change Breakpoint and Hi Tone. Try the blend.
3. Both effects have Output Level after Dry/Wet: for an unchanged dry comparison,
   set Level to 0 dB and Dry/Wet to 0%. Bypass is another comparison. They are
   distortion effects, not output limiters.
4. Automate a parameter in Pattern/Song mode, copy the chain, Undo/Redo a change
   and save/reopen. Listen at loop boundaries and after seeking. Use a spare
   project: schema 15 requires Build 267 or newer.

These are original effects, not measured Orion/ABL2 clones. Pro-9 is the next
requested generator; the current Drum Rack remains sample-based.

## Build 264: windows, automation and LFO

1. Insert Acid Bass and Drum Rack. Both controls should stay open. Drag one left
   and the other right; use their dock buttons without replacing/repositioning
   either. Double-click a generator name in the Playlist or Mixer footer to open
   it. Close/reopen one while the other remains visible.
2. Open **View > Event Editor**. Use its top-right Minimize, Maximize/Restore and
   X. Maximize should cover the workspace, while the transport stays accessible.
   Press Play/Stop repeatedly: the editor must stay visible until you close it.
3. The four top-left icons are Draw, Line, Erase and Move Graph. Cmd/Ctrl C/V/X,
   Z/Shift-Z and the Edit/context menus provide editing commands. Bottom-right
   **...** opens Settings: try Gradient fill and Red/Blue, Violet/Gold or Green.
4. Click the Event ruler to seek with Loop off: it must stay off. Drag a loop edge
   to resize, or drag inside its band to move the whole range. The main loop
   start/length controls and Playlist should agree.
5. **Record uses the mode beside the main Record button: MIDI or AUTO.** Use AUTO
   to capture knob movements in Pattern or Song mode. Opening Song Events selects
   AUTO. Press Record, move Acid Bass cutoff, then Stop; play back in the same
   mode. Mode changes finish a take. Return to MIDI before recording notes.
6. In Event Settings/context menu open **LFO Tool**. Change waveform, Speed,
   Amount, Offset, Phase, Fade In/Out and Curvature: the graph previews immediately.
   Sync on measures cycles across the target loop; off measures Hz. Cancel should
   preserve data; Apply should give one Undo. Song LFO replaces only the loop
   range. For Pattern automation, right-click its lane for the same tool.

## Build 263: original Peak Compressor

1. Select a generator with a playing loop, then **Insert > FX Dynamics > Peak
   Compressor** (or choose it in a Mixer insert). Start with Input/Output at
   0 dB, Detector EQ at 0 dB and Dry/Wet at 100%.
2. Lower Threshold until reduction appears. Increase Ratio: loud passages should
   become more controlled. At 1:1 and neutral gain the delayed signal is unchanged;
   INF is compression, not a guaranteed output ceiling at slow Attack.
3. Compare Attack 0.1 ms versus 30 ms on drums, and Release 30 versus 500 ms.
   Slow Attack lets more transient through; longer Release recovers more slowly.
4. Turn Detector EQ up/down: it changes compression sensitivity around 1 kHz,
   not the audible signal's EQ. Blend Dry/Wet and adjust Output to compare levels.
5. Choose another rhythmic generator as Sidechain. Its peaks should duck this
   track; disconnect to return to self detection. The trigger must not be mixed
   into this track. Use a short loop and listen for boundary clicks too.
6. Right-click Output for Song events and draw a ramp. Play in Song mode and
   check the level/readout follows. Drag a control, Undo/Redo once, copy the chain,
   then save/reopen and confirm parameters and sidechain. Use a spare project:
   schema 14 saves require Build 263 or newer.

This is original clean digital DSP, not a measured CV-82 clone. Listening and
matched Orion comparisons remain useful; no saturation or true-peak limiting is
claimed. Existing Compressor and Look-ahead Brickwall Limiter remain available.

## Build 261: look-ahead limiter

1. Insert **FX Dynamics > Look-ahead Brickwall Limiter** on the Master (select no
   generator, or use a Master insert slot). Place it after other processing.
2. Play an existing loop. Raise Input: louder peaks should produce gain reduction
   while the limiter output stays at/below 0 dBFS. Release changes recovery.
   Processing or Master gain after it may raise the final level again.
3. Bypass/re-enable, adjust a parameter, Undo/Redo, copy to another chain and
   save/reopen. Confirm settings and audible continuity. Listen at loop boundaries
   and seek during playback; report clicks/gaps separately from level readings.
4. This is an original sample-peak processor, not a claimed Orion emulation or
   true-peak limiter. Hardware listening remains a manual check.

## Build 260: Acid Bass and Drum Rack open together

1. Insert Acid Bass and Drum Rack. For each, open its generator triangle and
   **Open Separate Generator Controls**. Both windows should remain visible.
2. Choose A6 in the Drum Rack window, then click Acid Bass and adjust its cutoff.
   Return to the rack and draw steps/change velocity. Those edits should belong
   to Drum Rack A6; Acid Bass should retain its own pattern and cutoff.
3. Undo/Redo a control or drum gesture. Both windows should stay open with their
   own controls. Close Acid Bass: Drum Rack should remain. Reopen Acid Bass and
   check placement. Move/resize both using their native window controls.
4. Use either window's Piano Roll button: the shared Piano Roll should select that
   owner. Change a drum pattern there and check the detached rack follows it.
5. Save/reopen. Old control windows should close safely; reopen each through its
   generator menu and check instrument/pattern/sample state. This version does
   not provide two independent Piano Rolls or external VST instrument popouts.

## Build 259: VST3 effects and insert chains

1. Open **Insert > VST Effects > Scan / Manage**. Choose an installed audio effect
   and insert it on the selected generator. Use an empty Mixer/Master strip slot
   to choose a different target. Instruments should remain in the instrument list.
2. Click the effect name for its GUI. Right-click for **Host Parameters / Events**
   and sidechain/automation controls, or bypass/remove/reorder it. Serial order
   should follow the strip. Each insert/bypass/remove/reorder is undoable.
3. Copy an effect/chain, append to another strip or explicitly replace its chain.
   Check settings and distinct instances. Existing destination note/event data
   should remain. Overflow should refuse the whole paste. Undo restores the chain.
4. Fill four Master/return slots and also enable EQ/saturation/mute. All four
   effects should remain visible and process; internal nodes must not consume slots.
5. Save/reopen with the effect installed. Project metadata should allow loading
   without relying on a previous scan cache. A missing binary remains unavailable;
   this metadata does not install it or ensure old vendor chunks are compatible.

## Build 258: sidechain ducking and Master mute

1. Arrange a sustained bass and a kick pattern in Song mode. Add **Compressor**
   to the bass channel and open its effect controls. Click **Sidechain**, choose
   the kick channel, then lower threshold/increase compression. The bass should
   dip with kicks and recover between them. Choose None to return to self detection.
2. Undo/Redo the source choice, then save/reopen. The same source should return.
   Self-routing and feedback through other sidechains/buses should be refused.
3. Play parameter automation with its generic controls or Mixer visible. Controls
   should follow native values. A mouse drag should remain under your control;
   displays resume following afterward.
4. Automate Master level, then mute Master. The output must stay silent while the
   curve plays. Unmute: sound should return without deleting the curve. Add a
   Master insert while muted and confirm it stays silent. Save/reopen while muted
   and check again. Each mute/source change has one Undo transaction.

## Build 257: multiple buses, Master order and EQ warmth

1. On a playing generator, turn on Master plus Bus 1. Both route buttons should
   stay lit. Turn Master off: only the bus path remains. Add Bus 2 and change the
   bus faders independently. A bus must not route to itself or form a feedback loop.
2. Open Master Section: the four shared FX returns should be present, and only
   used output buses should show. Turn every dry destination off on the generator:
   the dry signal should stop, while an unlinked FX send can keep sounding.
3. In a new project, Master volume precedes inserts. Add a Master compressor and
   compare fader behavior before/after **Volume Before Insert Effects** in Master's
   right-click menu. Undo once restores the previous order. Existing Master volume
   and pan Song curves must keep working after the switch and after save/reopen.
4. Boost an EQ band, then right-click an EQ knob and enable **Soft Saturation**.
   Compare with it off. Bypassing EQ should bypass saturation too; re-enabling EQ
   restores the preference. Copy EQ settings to another channel and verify it follows.

## Build 256: Mixer settings and send routing

1. Adjust one channel's level/pan/EQ and FX send. Right-click its name label and
   copy Channel Settings. Paste onto another channel: those values should match,
   while its instrument, notes, insert effects, events and output bus stay intact.
   Undo once and Redo once should restore/reapply the complete paste.
2. Copy EQ Settings, then paste EQ onto a generator, return, bus or Master.
   Only EQ should change; level and pan must stay put. Rename a return/bus from
   the same menu, then save/reopen to confirm the name.
3. Right-click a send and uncheck **Linked to Channel Volume**. Lower the channel
   fader: its dry output changes while the unlinked send stays independent.
   Turn linking back on and both should follow the fader. Orbit's pan dependency
   follows the same choice because its native gain/pan stage is combined.
4. Right-click an EQ knob or an active send and open Song Events. Confirm the
   intended channel/parameter is selected. Pattern Events is available for a
   generator channel; Master/bus/return events belong to Song time.

## Build 255: Follow, fill and automation takes

1. Arrange a bass pattern over bars 1–8, set the Song loop to that range and play.
   Open **View > Event Editor**: its red cursor and gold loop boundaries should
   match the transport. Turn Loop off: it must stay off, with a dim loop range.
2. Enable **Gradient fill**. Zoom/scroll, toggle **Follow**, then watch a loop
   wrap. Follow should return to the loop start; turning it off lets you inspect
   elsewhere. These view changes should not consume Undo.
3. Right-click a host Acid Bass knob or generic parameter slider, choose
   **Edit Song Events**, then clear that lane. Press **Record** in the Song editor
   and move the knob while playing. Stop: the curve should appear. Undo once
   removes the complete take; Redo restores it. Save/reopen retains fine timing.
4. Select Master or an existing bus/FX return in the module selector and draw or
   record its level. Verify only that channel changes. Try a newly created send
   or EQ during a take and verify its first movement was captured.
5. Draw Pattern automation, place the pattern twice and play Song mode: the curve
   should restart at each occurrence. Draw Song events on that parameter: they
   override the Pattern curve. Clear Song events: the Pattern curve returns.
6. Choose **Options > Recording Settings > Automation** for main-toolbar Record;
   switch back to **MIDI notes** before testing note capture. Automation starts
   immediately; count-in remains a MIDI-note feature. Listening and physical MIDI
   timing need manual checks, particularly across loop boundaries.

## Build 254: Song automation

1. Put a bass pattern in the Playlist for four bars. Choose **Song** mode.
2. Open **View > Event Editor** (or right-click its Playlist track and choose
   **Edit Song Events**). Choose that track and a **Filter Cutoff** parameter.
3. Choose **Line** and drag from low at bar 1 to high at bar 5. The graph should
   cover the full parameter range. Play: hear the filter opening over the song.
4. Undo once, then Redo once. The complete drag should disappear/reappear.
   Draw a curve and press Escape before releasing: that edit should be canceled.
5. Try **Erase**, **Move Graph**, Copy/Paste, Clear, Smooth and right-click Scale.
   Copy/Paste transfers the complete curve at the same song positions. The note
   clipboard and Pattern event lanes should remain unchanged.
6. Save/reopen. The curve should return on the same plugin instance. Two copies
   of the same effect should offer distinct slot entries and independent curves.
7. Switch to Pattern mode: Song automation should stop driving those parameters.

Build 255 adds live recording and Master/bus targets; tempo automation remains pending.

## Build 252: Mac clipboard and steady view

1. Draw a few notes in A1. Click in the Piano Roll and press **Cmd+C**.
2. Choose A2, scroll so C4 is at the top, and press **Cmd+V**. The notes should
   paste with their original pitches and timing; C4 should stay at the top.
3. Press **Cmd+Z** to undo and **Cmd+Shift+Z** to redo. Repeat without clicking
   the grid again. The view should stay steady and the shortcuts keep working.
4. Select some notes, then try **Cmd+X** and **Cmd+V**. Only the selected notes
   should be cut/pasted. **Cmd+A** selects all notes.
5. Try ordinary text entry in a rename/settings field: its own copy/paste/Undo
   should work without editing the pattern.

## Build 251: count-in and loop recording

1. Start with a new project and a Built-in 4OSC Synth. Choose **Pattern** mode.
   Open **Options > Recording Settings...** and select **4**. Leave METRO off.
2. Open Piano Roll, press Record and listen for four clicks. Click a key during
   the count-in: it should sound, but should not add a recorded note.
3. After the count-in, click/hold/release a key. Leave Record running. You should
   hear that note again on the following loop. The pending note appears in the roll.
4. On another pass, record a different key. Both phrases should repeat. Stop,
   then Undo once: the complete take should disappear, leaving earlier notes.
   Redo and save/reopen to check the finished take.
5. Start Record again and Stop before counting finishes. It should add no notes
   or Undo entry. Set count-in to 0 and confirm immediate recording.
6. Enable METRO and try again: it should keep clicking after the count-in.
   Recording started while playback is already running skips the count-in.

This is Pattern overdub. Song looping, audio input recording and automation
recording remain separate pending work. Hardware timing still needs real-device tests.

## Build 249: Playlist group editing

1. Paint A1 at bars 1 and 3, plus a pattern on a second instrument. Click the
   Playlist **Select arrow**, then drag a rectangle around all three clips.
   Their borders should turn gold.
2. Drag a selected clip four bars right. All three should keep their spacing and
   lengths. Undo once, then Redo once.
3. Hold Ctrl (or Cmd on Mac) and drag the selected group to empty space. The
   originals should remain. Undo once removes the complete copy.
4. Try dropping onto occupied clips or outside the track list. The entire move
   should be refused. No clip should disappear or move on its own.
5. Shift-click toggles one clip. Delete removes selected placements, preserving
   their source patterns. Undo brings the placements back.
6. Hover a clip, with Playlist focused, and press N. Rename it. All placements of
   that source pattern should show the name; other patterns should stay unchanged.
7. In Playlist Settings enable Follow Play Cursor and Show Pattern Velocities.
   Play past the visible area and check that it follows. Shift+S toggles Follow.
   Change Navigation Bar, quit/reopen, and check that the choices persist.
8. With an audio clip, move/copy it to an empty part of an audio track. It should
   play the same source excerpt. Check one Undo and project save/reopen.

These are whole-bar arrangement edits. Song automation remains at its original
arrangement times; fine snapping and the remaining recording work are pending.

## Five-minute editing test

1. Open **Piano Roll** above the instrument. Draw C4, E4 and G4 at the same
   starting step. This is one chord with three separate notes.
2. Click **VEL** at the bottom right. Drag only C4 up/down. Its velocity number
   and shade should change; E4/G4 should stay unchanged. Undo should restore C4
   in one action. Turn VEL off before continuing.
3. Shift-click C4 and E4 to select them. Drag one selected note to the right and
   up. Both selected notes should move together; G4 should stay put. Undo once.
4. Ctrl-drag (Cmd-drag on Mac also works) that selected group. The originals
   should stay, with a second pair where you release. Undo should remove the copy.
5. Hold **Alt** and drag through empty grid cells. You should get separate notes
   using the current grid and note length. Undo should remove the whole stroke.
6. Click **Select** at the top left (or press T), then drag a rectangle around
   notes. Click **Draw** to draw again. In Draw, Shift-click or Shift-drag can
   select without changing tools. Escape clears the selection.
7. Open **View > Hotkeys…** for a searchable reference to implemented shortcuts.
8. Click the header Restore icon while the Piano Roll is maximized. It should
   become a floating, resizable Piano Roll. Maximize/restore should remember that
   size; **Instrument** should still return to the synth.

## Drawing and velocity regression checks

- Click near the left, middle and right of steps 1, 4, 7 and 11 on several pitches.
  Each click should create a note in that cell. Try 16, 32 and 64 grid subdivisions.
- In VEL mode, drag a note upward/downward to reach 127 and 1. Other chord notes
  should keep their values. Pointer travel continues at the screen edge.
- Show the velocity lane with the bar icon. A chord has separate bars, ordered
  by pitch from left to right within its onset. Drag one: its matching note and
  bar turn gold, and the lane shows its pitch/value. The top gives 127 and the
  bottom gives 1. Other chord notes should not change. This lane works with VEL off.
- For the reported weak-range case, draw three simultaneous notes. Give one
  velocity 1, another 64, another 127. Their bars should reach the bottom, middle
  and top independently. In VEL mode, drag the first note and watch its own bar
  span the entire lane. Undo each stroke once.
- Insert several VST instruments, including large GUIs, and switch their GUI /
  Piano Roll views. The title, bank/slot controls and instrument button should
  remain above the plugin. Test GUI resizing or preset changes that resize it.

## Other Patterns features

| Test | What to do | Expected result |
| --- | --- | --- |
| Scissor | Select a long note and press S with the grid focused | It splits in half; Undo joins it back |
| Pitch editing | Select notes; press Up/Down, then PgUp/PgDn | Semitone, then octave moves; out-of-range moves are refused |
| Rotation | Press Left/Right on a phrase | Notes wrap by one sixteenth within the pattern |
| Flips | Select a phrase; try X, Undo, then Y | X reverses time; Y mirrors pitches |
| Rename/slots | Press N; enter a name. Press 1–8 | Name changes; slot keys select within the current bank |
| Ghost notes | Draw a phrase on a second generator, return to the first and enable Ghost (M) | The second generator's remembered phrase appears faintly and cannot be edited from here |
| Per-generator settings | Set one generator's grid to 8 subdivisions, another to 32; switch between them, save and reopen | Each keeps its own setting |
| Scale snapping | Gear > enable Snap to scale, C minor; draw or move notes | New pitches follow C minor; existing off-scale notes remain editable |
| Edit audition | Gear > enable Edit audition; draw/move a note | It sounds briefly. This edit audition does not record a duplicate note |
| Follow/cursor | Gear > Follow Play Cursor; play a long pattern. Then disable Show Play Cursor | The time view follows playback; cursor visibility can be changed independently |
| Automation LFO | Enable Automation, choose a target, right-click its lane and open LFO | Preview changes the display; Cancel preserves data. Apply changes the lane, with one Undo |

## Generator and Playlist checks

- Repeat the 09-15-07 recording: insert Ostirus or Labyrinth, then drag slowly
  and quickly in circles, vertically and horizontally. Watch for a second jump
  after each pointer movement; the toolbar and GUI should follow one trajectory.
  Repeat while Mixer/Playlist overlap, and from the Piano Roll’s inner title.
- Re-select the already selected generator from its dock button. It should keep
  the same native editor. Switching to another generator still opens that editor
  normally. Compare cold insertion separately from re-selecting an open generator.

- Open Mixer and Playlist so they overlap a floating generator. Drag the
  generator’s outer title horizontally, vertically and back across both panels.
  Its toolbar/body should move together and stay above them during the gesture.
- Open Piano Roll and repeat using its own inner title. The roll must stay flush
  inside the generator; repeated back-and-forth moves must not accumulate offsets.
  Try its resize grip too. Click Mixer or Playlist afterwards: that entire panel
  should come forward, without a plugin GUI drawing through it or stealing clicks.
- With RipplerX or another native VST3, switch GUI/Piano Roll repeatedly. Test a
  large GUI with Auto Fit, then resize the host. The toolbar should remain above
  the GUI, and the complete instrument should fit below it. Bring Mixer/Playlist
  over it and move them: covered plugin controls must not receive their clicks.

- Open/close Piano Roll using the button above the instrument. The old GUI
  should disappear while the roll is shown. Returning should restore the
  instrument's size. Undo while the roll is open should preserve this behaviour.
- The top-left triangle opens Generator Settings and Arpeggiator/Shuffle for
  instruments and Drum Rack alike.
- A maximized Piano Roll or Playlist should meet all workspace edges below the
  transport and above the status bar, including after resizing the app window.
- Drag the three-line grip beside the Playlist Generator heading left/right.
  The column should resize, even in an empty project, and remember the width.
- Click an existing Playlist block, hold and move the mouse up/down. Its pattern
  should change. The wheel is another way to do this. Playlist Settings can
  restrict choices to existing patterns; otherwise blank slots are included.
- Drag horizontally in Draw mode to stamp pattern-sized copies. To extend a
  placement, deliberately drag its right edge. Normal painting should not stretch
  one placement accidentally.
- Click the Playlist ruler to seek. Loop should stay off. A deliberate ruler
  drag selects/enables a loop.

## Acid Bass: quickest sound test

1. Insert > Synths > **Acid Bass**, then click **Demo pattern**. It opens a fresh
   pattern with notes and switches. Existing music is kept in its original slot.
2. Select Pattern mode and press Play. Start around 120–140 BPM.
3. Toggle some **Accent** cells under the grid. Accented notes should become
   more pronounced/brighter. Turn the Accent knob down/up to hear its strength.
4. Toggle **Slide** on a note before a different pitch. The transition should
   glide and keep the envelopes connected. Two equal pitches should tie rather
   than pluck again. Try the Slide knob for faster/slower transitions.
5. Return to Instrument and turn **Cutoff**, **Reso** and **Env Amt**. Switch Saw
   to Square. Acc Env changes how quickly the accented filter movement recovers.
6. Save/reopen and check the sound settings and switch rows. Undo should restore
   a switch stroke or knob gesture in one action.

If something fails, report the build label, the test step, and what happened.
Audio-device latency and physical MIDI input need separate real-device tests.
