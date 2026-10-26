# Orbit Orion-style visual targets

Use this note as the ongoing UI reference when adjusting the generator, piano roll, playlist, and mixer. Refer to the attached screenshots in this conversation and these project reference files:

- `ORION_LEARN_NOTES.md` — timestamped observations from the Orion screen recording.
- `ORION_LEARN_TODO.md` — high-priority, detailed Orbit implementation backlog grounded in that recording and the user's earlier screenshots/requirements.

- `Screenshot 2026-10-25 at 9.52.26 PM.png` — generator toolbar and the preferred piano-roll window proportions.
- `Screenshot 2026-10-25 at 9.53.04 PM.png` — current piano roll with too much unused space to the right and an awkwardly tall floating window.
- `Screenshot 2026-10-25 at 9.58.12 PM.png` — mixer detached as a separate window; desired mixer floats above the Orbit workspace without replacing it.
- `Screenshot 2026-10-25 at 10.01.10 PM.png` — transport/menu alignment; the dark transport band must fully contain its controls and stay compact.
- `MIXER.jpg` and `8512af06-78f6-426d-a711-0e62cd1bfa38.jpg` — target channel-strip grouping, spacing, and panel style.
- `videoframe_72931.png` — Orion pattern-generator toolbar order and piano-roll reference.
- `gerner.jpg` — annotated comparison showing the generator panel should own its pattern controls and instrument interface as a compact, divided unit.

## Generator and instrument interface

- Treat each generator as a recognizable, self-contained panel. Keep its name, mute/solo, A–H bank row, 1–8 slot row, piano-roll action, settings, preset controls, and pattern length together.
- Give the pattern controls a clear panel boundary and internal dividers. Avoid stretching them across the whole host window or leaving a wide undifferentiated blank area beside them.
- Place the selected instrument's GUI in a distinct, labeled instrument area directly beneath the pattern controls, inside the same generator panel. Show the built-in 4OSC interface for the current instrument; use the same area for a VSTi/AU interface when plugin hosting is implemented.
- Keep the piano roll inside the generator panel rather than in a separate window. It replaces the instrument GUI area while open, but leaves the generator name and A–H / 1–8 selectors visible. Changing the selected pattern updates the open piano roll immediately.
- Keep the mixer as a separate floating window above the Orbit workspace, so opening it does not replace the generator or piano roll.
- Keep the visual hierarchy in Orion's order: generator identity and pattern bank first, instrument interface beneath; the piano roll is an editor associated with that generator and selected pattern.

## Piano roll

- Open centered at a useful landscape size, visually attached to the Orbit workspace rather than an oversized, tall utility window.
- Show the piano keyboard, editable note grid, and velocity lane together where practical. Avoid a large unpainted area at the right edge.
- The selected pattern length defines the active steps. Draw a clear red boundary at the active end; show any future steps beyond it with darker inactive cells. Extending the pattern length should reveal those cells as active, in step increments.
- Keep the default pattern view at 16 steps; use adjacent decrement/increment controls to extend the pattern one step at a time up to 64. Note lengths persist with the pattern, and dragging a note extends its duration. Right-click removes notes; a normal click on an existing note must never erase it.
- The piano roll must remain available while the mixer is visible, so users can refer to both views together.

## Mixer and host layout

- The mixer opens as a separate floating window over the Orbit workspace. Keep the generator and its open piano roll visible behind it.
- Keep the mixer view sized to the host content area and show only actual generator channels plus Master. Let the channel viewport scroll horizontally when necessary.
- Keep the menu and transport rows compact. The transport's dark background must extend behind the full height of its controls so buttons do not touch or cross the content background. Keep the status/footer strip shallow.
- The Playlist has compact zoom out/in controls. Activating an empty cell places the selected pattern slot directly; dragging an existing clip may move it to another bar or generator row.
- Give each Playlist generator row compact Mute, Solo, and Record-arm controls. Provide a Pencil/Draw tool that paints the selected pattern across empty cells by click-drag, and keep a separate Move/Select tool for repositioning clips.
- Playlist zoom is split into independent horizontal timeline zoom and vertical track-height zoom controls. Keep the Pencil/Move and zoom controls grouped in the Playlist header until a later Orion toolbar reference specifies the exact placement.
- Continue tuning channel-panel divisions, bevels, knobs, sends, inserts, EQ graph, mute/solo, fader, and meter against the supplied mixer references.

## Visual implementation choice

Prefer JUCE drawing for interactive surfaces, gradients, panel edges, knob geometry, indicators, and meters. It stays crisp when resized and keeps controls responsive. Use PNG assets only when a specific texture or hand-crafted artwork cannot be reproduced cleanly with drawing code; avoid using a flattened image for controls that need interaction.
