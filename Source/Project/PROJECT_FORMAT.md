# Orbit Project Format — Schema 6

Orbit owns the project document format. Tracktion Engine is currently the live
audio and MIDI engine behind an adapter; stable Orbit IDs identify project
objects and mixer channels independently of Tracktion's internal IDs.

## File and compatibility

- Project files use readable XML with the `.orbit` extension.
- The root element is `OrbitProject` and carries `schemaVersion`.
- Schema changes that alter interpretation require a version increment and a
  migration before the project is passed to the engine adapter.
- A newer unsupported schema is rejected without modifying the file.
- Save writes a sibling temporary file and replaces the destination only after
  the new XML has been written.

## Current structure

- Project: stable UUID, display name, and `ticksPerQuarterNote` (960).
- `TempoMap` and `TimeSignatureMap`: stable point IDs, integer tick positions,
  and tempo or meter values.
- `Tracks`: stable IDs, kind, display name, order, and a reference to each
  track's mixer channel. Instrument tracks own 64 named pattern slots (A1–H8)
  and pattern note data. Track order is presentation, never identity. An
  optional `showGridNotes` boolean stores the track's Playlist piano-grid
  preview preference; older files without it default to hidden.
- Track children reserve separate `Clips`, `Plugins`, and `AutomationLanes`
  collections. Playlist clips refer to pattern IDs rather than list positions.
  A pattern clip stores its bar span and whether it follows its source pattern's
  length; legacy clips without that flag follow the pattern length, with a
  minimum of one bar. Schema 5 adds an optional `exactLength` boolean: when
  true, `lengthBars` is the exact range, even if shorter than its pattern, and
  `followsPatternLength` is false. Without it, previous minimum-pattern-length
  behavior is preserved. This allows short pattern placements without shortening
  the source pattern. Clone/import retains the flag. New readers migrate the
  root schema version on load; readers predating schema 5 reject these clips
  instead of silently expanding them.
- Schema 6 defines pattern `Note` children as an ordered list of independent
  records. Multiple records may share pitch and exact start tick; their individual
  durations and velocities survive clone/save/load and engine staging. The strict
  append API validates bounds and never merges coincident notes. The older
  single-note setter retains its upsert behavior for existing callers.
  Piano-roll move, resize, velocity and erase operations at a shared onset edit
  that onset's stack together; moving onto a different existing onset is refused.
  Whole-pattern clipboard and bulk-edit intermediates use native ticks and all
  MIDI pitches (0–127), including notes outside the editor's current display range.
  Deliberate quantize/scale operations change the requested timing fields;
  passthrough fields retain their tick values. Older supported files migrate to
  schema 6 without rewriting existing note values. Readers supporting only schema
  5 or earlier reject schema 6.
- `MixerChannels`: stable IDs and output links for generator channels, FX
  return channels, and Master. Generator records include gain, pan, mute/solo,
  and four FX send levels. FX return records are keyed by their stable bus
  number and include gain, pan, mute/solo.
- `TracktionEditState`: a serialized Tracktion edit used to restore the current
  audio graph, MIDI clips, plug-in state, and engine routing. Orbit mixer
  records mirror stable channel identity and key controls alongside that
  engine state.
- `MediaReferences`: reserved for external audio and sample references.

Project timeline positions use integer ticks at the project's 960 PPQ. The
current pattern-note representation separately uses 64 ticks per sixteenth note
(256 ticks per quarter), and pattern lengths use sixteenth-note steps. Playlist
ranges currently use whole 4/4 bars. Import adapters must convert explicitly
between these units; they are not interchangeable. Audio positions
use sample/frame units in the engine adapter and media model; they must not be
serialized as floating-point beats or mixed with musical tick values.

## Current implementation notes

The app creates Tracktion-backed instrument tracks, routes sends to shared FX
return tracks, and saves the Tracktion edit beside Orbit's project model. FX
return channel IDs are stored on their Tracktion tracks and mirrored into the
stable Orbit mixer-channel records. Portable media relinking, a project-level
plug-in identity model remain future work. Native project/session snapshot undo
and the isolated importer transaction adapter are implemented. Native patterns
are authoritative when reopening a project: the legacy engine-to-grid migration
runs only on tracks without created native patterns or native note records.
This prevents authored note stacks and short placements from being reconstructed
through the older boolean step grid. The engine's existing non-MPE overlapping
note-off policy still applies; storing separate records does not establish Orion
voice/release behavior. Live SFS import/undo/redo was verified in isolated Build
212; later native reopen/edit checks use a temporary headless host-method harness,
with display centering and modal UI/setup/answers bypassed for that harness.
