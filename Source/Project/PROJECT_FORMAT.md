# Orbit Project Format — Schema 2

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

## Schema 2 structure

- Project: stable UUID, display name, and `ticksPerQuarterNote` (960).
- `TempoMap` and `TimeSignatureMap`: stable point IDs, integer tick positions,
  and tempo or meter values.
- `Tracks`: stable IDs, kind, display name, order, and a reference to each
  track's mixer channel. Instrument tracks own 64 named pattern slots (A1–H8)
  and pattern note data. Track order is presentation, never identity.
- Track children reserve separate `Clips`, `Plugins`, and `AutomationLanes`
  collections. Playlist clips refer to pattern IDs rather than list positions.
- `MixerChannels`: stable IDs and output links for generator channels, FX
  return channels, and Master. Generator records include gain, pan, mute/solo,
  and four FX send levels. FX return records are keyed by their stable bus
  number and include gain, pan, mute/solo.
- `TracktionEditState`: a serialized Tracktion edit used to restore the current
  audio graph, MIDI clips, plug-in state, and engine routing. Orbit mixer
  records mirror stable channel identity and key controls alongside that
  engine state.
- `MediaReferences`: reserved for external audio and sample references.

All musical positions are integer ticks at the project's PPQ. Audio positions
use sample/frame units in the engine adapter and media model; they must not be
serialized as floating-point beats or mixed with musical tick values.

## Current implementation notes

The app creates Tracktion-backed instrument tracks, routes sends to shared FX
return tracks, and saves the Tracktion edit beside Orbit's project model. FX
return channel IDs are stored on their Tracktion tracks and mirrored into the
stable Orbit mixer-channel records. Portable media relinking, a project-level
plug-in identity model, and undo transactions remain future work.
