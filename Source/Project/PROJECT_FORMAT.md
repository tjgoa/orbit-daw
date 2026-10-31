# Orbit Project Format — Schema 20

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

Schema 20 appends Acid Bass `fxTriangleFold` (0=Off, 1=On, missing=Off). The
new shape uses the same oversampling latency/dry alignment. Original `fxMode`
remains 0..1 Soft/Sine Fold, including its normalized automation; the separate
triangle switch overrides that shaping while enabled and leaves the saved old
mode/curve intact. The UI combines them into a three-choice selector. Voice/FX
history is transient. Pre-schema-20 readers reject new saves; supported old
projects migrate with Triangle Off and retain their original mode/automation.

Schema 19 appends the native Acid Bass `phaseRetrigger` parameter after the
existing fifteen parameters. Discrete values 0/1 mean Off/On, default 0 for old
presets/projects. It resets only oscillator phase on fresh non-legato attacks;
phase/filter/audio history remains transient. The existing parameter IDs and
Soft/Sine Fold range are unchanged, so existing normalized FX automation keeps
its interpretation. Older supported projects migrate normally; pre-schema-19
hosts reject new projects rather than silently dropping the new setting.

Schema 18 adds the registered native instrument `orbitNineKitAudition`. Its 98
stable native parameter IDs use the pinned upstream pot names plus
`orbit_step_accent` and `orbit_note_map`; the IDs/choices are generated in
Source/Synth/NineKitMetadata.h. Notes use the selected MIDI map. Step Accent
reuses the independent pattern switch records (slide=false) and generates CC119
before simultaneous notes; sample/FX/voice histories are transient. Controls,
instance identity and automation persist in the companion engine edit state.
The three immutable source samples are compiled resources, with no project file
references. Existing supported projects migrate without changing their notes;
older builds reject schema 18 instead of silently dropping the new instrument.

Schema 17 adds the registered native instrument `orbitGeonkickAudition`. Its
stable parameters are sound, tune, decay, sweep, noise, tone, drive and level.
They are retained in the companion Tracktion edit state; automation addresses
the native parameter IDs. Older projects migrate without changing their audio.
Older Orbit builds reject schema 17 rather than silently dropping this synth.

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

## Optional metadata added in Build 244

Pattern `Articulations` children contain `Switch` records with `startTick`,
`lengthTicks` and `slide` (false means Accent). Intervals use pattern ticks,
remain bounded by pattern length, and survive native copy/Undo/save/reopen.
These optional independent records leave schema-6 note interpretation unchanged.
Acid Bass state is the native Tracktion plugin type `orbitAcidBass`.

Missing imported audio clips retain `orbitOriginalReference`,
`orbitExpectedRate`, `orbitExpectedChannels`, `orbitExpectedFrames`,
`orbitExpectedFormat`, `orbitExpectedBits` and `orbitMissingMedia` in the engine
clip state. An empty source is an intentional silent placeholder with its
original timing. Folder relinking clears the missing flag after a unique valid
match; the expected metadata remains available. Sampler media remains in the
engine's sound state. Missing external effects keep their plugin state when
bypassed. `MediaReferences` is still reserved; this pass uses engine metadata.

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
# Piano Roll metadata and pattern automation — Build 242

Engine audio tracks may contain an optional `PIANOROLL` child with editor grid,
display, follow/audition and root/scale preferences. It is stored with the native
engine state, independent for each generator. Older files without it use current
application defaults; no schema-version bump or Orion-format change is involved.

Before schema 9, Pattern automation point positions used integer sixteenth-note steps. New
edits are bounded by the selected pattern length rather than a fixed 4095-step
ceiling. Existing stored lanes are not silently truncated on load. LFO lane
replacement preserves the lane identity/target and other lanes, builds a sorted
normalized replacement before attaching it, and uses the host's single Undo
snapshot. The GUI LFO samples at most 4096 points across the full pattern.


## Song automation instance identity — schema 7

Song `AutomationLane` records may carry `pluginId`, the persisted Tracktion plugin
instance ID, alongside `pluginName`, `parameterId` and `parameterName`. Positions
are sixteenth-note steps on the song timeline (0–65535), values normalized 0–1.
Empty instance lanes suppress older name-only Song data. Since Build 255, Clear
restores arranged Pattern automation when present, otherwise removes the curve.
A lane with an instance ID takes precedence over a legacy name-only lane.
Legacy lanes are read only when the plugin name/parameter identifies one instance;
ambiguous legacy data remains in the document without driving multiple plugins.
Schema 0–6 files migrate structurally without assigning guessed instance IDs.
Native Merge remaps plugin IDs with the imported engine track. Clone Track To
maps same-name plugin occurrences onto the existing destination plugins; unmatched
instance lanes remain retained but inactive. Schema 7 saves require Build 254 or
newer, preventing older builds from misinterpreting instance lanes as name lanes.

Song Event Editor gestures preview locally, then replace a complete lane as one
project Undo transaction. Pending gestures are canceled on project/target changes
and rejected if the source lane changed. They do not create Pattern events.

## Precise automation and mixer modules — schema 9 / Build 255

Automation `Point.position` is a finite double in sixteenth-note step units,
retaining sub-step native performance timing. Integral older positions load
unchanged. Pattern replacements stay below their pattern length; Song replacements
stay below step 65536. Values remain finite normalized 0–1. Cell-based drawing
replaces all points inside that cell while preserving fractions elsewhere.

Optional root `SongAutomationModules` children named `Module` own mixer Song lanes.
Their stable IDs are `mixer:master`, `mixer:fx:0`–`mixer:fx:7`, and
`mixer:bus:0`–`mixer:bus:7`. Each module has the same `AutomationLanes` layout as a
project track, including exact `pluginId` targeting. Tracktion's final Master
volume is enumerated separately from its insert list. Older supported schemas
migrate on load; schema 8 was an unpublished intermediate. Build 254 and earlier
reject schema 9 rather than quantizing fractional points or ignoring module data.

Engine plugin state may cache normalized gap baselines under
`ORBITAUTOMATIONBASES/PARAMETER(parameterId,value)` for arranged Pattern curves.
These are playback baselines, separate from the authoritative authored lanes.

## Output routing and Master order — schema 10 / Build 257

Engine audio-track state may carry `orbitOutputRouteMask`, a 9-bit integer:
bit 0 is direct Master and bits 1–8 are Bus 1–8. One destination uses the native
track output; additional destinations use tagged `orbitOutputRouteSend` aux
plugins on buses 32–39 and corresponding `orbitOutputBusReturn` plugins. Those
bus numbers are separate from ordinary FX send buses 0–7. With mask 0, a private
`orbitOutputGate` plugin tagged `orbitRouteDryMute` silences the final dry branch
after sends. An empty engine output name is not used, because it falls back to
Master. Feedback checks consider direct outputs, enabled aux routes and sidechain
edges before committing changes.

`orbitSendPreFader0`–`orbitSendPreFader7` booleans on a track store send volume
linking, including inactive sends. Native send position before/after the channel
volume/pan plugin carries the actual processing order.

A regular native volume/pan plugin tagged `orbitMasterPreInsert` at the start of
the Master insert list implements pre-insert Master level. The engine's separate
final Master plugin is neutral in that mode. Without this tagged plugin, the
saved final-stage order is preserved. Switching mode transfers curves and values
and remaps both plugin IDs and the differing native parameter IDs (`volume` vs
`master volume`, and their pan counterparts) in Song lanes. Gap-baseline cache
keys move with those IDs. New projects use pre-insert order; older files keep
their saved order when loaded.

Optional `orbitEqSaturation` plugin state is a private post-EQ soft saturation
stage. `orbitEqSaturationOn` retains its preference while EQ bypass disables its
processing. Settings clipboard transfers this option, not the companion's ID.
Schema 10 protects these processing/control semantics from older readers.
Build 256 and earlier reject new schema-10 saves; older supported projects load.

## Automation-safe Master mute — schema 11 / Build 258

`orbitMasterMuted` remains the Master mute flag, but now controls a final private
`orbitOutputGate` plugin tagged `orbitMasterMuteGate`. It follows all Master
inserts, before the engine's neutral/final Master volume stage. Master volume/pan
curves and manual values are not overwritten to implement mute. Adding an insert
keeps it before the gate. Unmute removes the gate, preserving the active curves.
Older muted states with no gate and a lowered fader recover their
`orbitMasterPreMuteGainDb` value behind the gate. Schema 11 prevents older builds
from retaining this gate after using their earlier unmute implementation.

Sidechain routing uses the native plugin `sidechainSourceID`, native connection
wires, and Compressor `sidechainTrigger` state. It does not add a second Orion
format. Unsupported/missing source IDs remain retained until an explicit change.

## Effect graph budget and plugin catalogue — schema 12 / Build 259

User-visible inserts retain three generator slots and four Master Section slots.
The engine budget is separately 128 plugins per track and 64 in Master, allowing
private routing, gain/EQ/saturation/mute nodes alongside those inserts. This
schema prevents older builds from editing a larger valid graph with the former
four-total-Master-plugin limit and silently failing an internal insertion.

Optional native edit child `ORBITPLUGINCATALOG` contains `ENTRY(key)` records,
each with JUCE's XML PluginDescription subtree. It stores class/format/file/role
metadata, not DSP binaries or processor-state replacements. Existing entries for
unavailable plugins are retained; loaded plugin descriptions update matching
entries. Before native edit creation, installed local VST3 descriptions are added
to the host catalogue. Older files without it resolve only exact saved class IDs
(or a unique name if no ID exists) from existing local VST3 paths. Missing and
ambiguous matches are not guessed. Ordinary native plugin state remains the
owner of processor chunks, parameter curves, bus layouts and instance IDs.


## Native look-ahead limiter — schema 13 / Build 261

Native engine plugin type `orbitLookaheadLimiter` stores automatable `inputDb`
and `releaseMs` properties. Delay/queue history and input/reduction meters are
transient and are not serialized. Build 261 registers the processor before Edit
creation. Schema 13 prevents an older host from dropping this native effect
under its unknown-plugin behavior; supported earlier schemas still migrate.


## Original peak compressor — schema 14 / Build 263

Native plugin type `orbitPeakCompressor` stores `inputDb`, `thresholdDb`, `ratio`,
`outputDb`, `detectorEqDb`, `attackMs`, `releaseMs` and `mix` (percent), along with
standard native sidechain IDs/wires and automation. Ratio 100 selects infinity.
Delay/filter/gain history and meters are transient. Build 263 registers the new
type before Edit loading; older supported schemas still load. Schema 14 prevents
older hosts from discarding an unknown native effect. No existing `compressor`
state or proprietary Orion effect state is reinterpreted as this processor.


## Original distortion effects — schema 15 / Build 267

Native plugin types `orbitTubeDistortion` and `orbitTwoBandDistortion` retain six
native parameter values each. Tube IDs: drive, lowCut, highDamp, shape, level, mix.
Two-Band IDs: breakpoint, driveLow, driveHigh, highTone, level, mix. Their stable
instance IDs distinguish Song automation and remap on chain copy. FIR/filter/DC
history, smoothing and dry delay are transient. Schema 15 protects the new types
from older hosts; existing native compressor/synth types are preserved.


## Acid Bass internal FX — schema 16 / Build 270

Six native parameters are appended after the existing nine Acid Bass parameters:
`fxDrive`, `fxDistort`, `fxLowCut`, `fxTone`, `fxOutput`, `fxMode`. Missing parameters
use the documented defaults; old projects remain dry at Distort 0%. Existing
parameter IDs and automation targets retain their meaning. Schema 16 prevents
older builds from accepting a project and silently dropping new FX settings.
The oversampler, filters, dry delay and smoothed values are transient audio state.
