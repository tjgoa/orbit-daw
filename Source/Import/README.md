# Isolated SFS extraction and experimental pattern recovery

The SFS parser is original standard-library C++. It does not embed Orionology,
Synapse headers, plugin binaries or DSP. The preview UI uses JUCE; the narrow
native adapter depends only on the format-neutral plan and ProjectDocument.
The host stages its Tracktion session separately and contains no SFS record types.

## Recovery dialog follow-up — Builds 243–244

Choose **File > Merge Project...** and select the SFS. The dialog starts at
**96 PPQ / 4/4**, as requested, with editable fields for other source settings.
Tempo may show a tentative source candidate and still needs confirmation.
Whole generators are selected by default, including all their recoverable
patterns. Cmd/Ctrl-click excludes unwanted generators. Explicitly choose the
recovery policies. **Merge selected
content** now validates and imports directly; **Preview recovery** is optional.
Any refusal appears above the buttons as well as in the report, including host
staging failures. Changed fields/selection invalidate the old plan/report.

With **Keep missing audio as silent placeholders** (default), accepted dry-audio
recovery can merge without all WAVs. Original paths, expected metadata and clip
positions survive save/reopen. File > Find Missing Media searches a chosen folder
and subfolders for unique filename/metadata matches. Ambiguous or incompatible
files remain unresolved. Without placeholders enabled, unresolved WAVs still
block selected Audio rows; deselect Audio for notes-only recovery. Checking all
options does not locate files. Song1 A2 still needs explicit omission of its
unsupported event records. These controls do not extend binary format coverage.

Folder relinking also handles native missing WaveAudioClips and Sampler sound
references. No samples are copied or synthesized. File > Bypass Missing Effects
disables unavailable native external effects and keeps their saved state; the
SFS converter continues to omit Orion effect/instrument state.

Reproduce the focused dialog check using the local Song1 fixture (not included):
`python3 Tools/test-sfs-preview-ui.py /path/to/Song1.sfs`.
It exercises explicit consent, preview/direct merge, invalid timing, changed
settings, missing media, visible host failure/retry and the 32-note/62-placement
notes-only plan with two retained unsupported records. It renders the dialog
without opening a native window or committing to a user's session.

## Implemented format coverage (verified 2026-10-28, Build 220 / v0.3.199)

- Read-only inventory and generator pattern framing for versions `0x8000` and
  `0x8612`, exercised on reference TestSong and user Song1.
- A tentative initial BPM candidate from GLOB+12 for the two observed version/size
  layouts. Raw float bits and source span are retained, including invalid values.
  Only a finite positive value is offered in the preview; it must be confirmed.
  The converter still requires supplied meter and PPQ values; the current dialog
  prefills the user-requested 96 PPQ / 4/4 defaults, which remain editable.
- Observed `0x8000` / module `0x1003` / 208-byte metadata framing: a bounded
  extension byte count at payload+64 precedes a validated track-data chunk and
  the ordinary pattern sequence. Both reference instances are retained as
  opaque prefix spans; no plugin state is loaded or converted. This extension
  mapping is not generalized to other versions/layouts.
- Raw words and source spans for all pattern records. Code `0x3002` remains a
  **tentative** note interpretation; conversion requires explicit acceptance.
- Parsing and inspection use one immutable in-memory byte snapshot;
  non-overlapping `opaqueRecords` retain every source byte. Song-info
  declared and consumed sizes are distinguished. Retention is not semantic
  coverage, and opaque bytes must never be executed.
- PATC 16-byte raw framing and an opt-in tentative `0x3007` change mapping for
  the known versions. Module/pattern references are validated;
  unknown fields/codes and unassignable module references block MIDI arrangement
  conversion. Invalid known pattern references or simultaneous changes block
  recovery of their selected track; explicitly unselected tracks can be omitted.
  Ranges end at the pattern end or next change on the same module. Changes to
  unselected patterns still bound preceding selected placements; gaps remain.
- Schema 5's generic native `exactLength` clip flag preserves short placements
  without shortening source patterns. Legacy spans retain prior behavior. The
  adapter currently requires whole-bar ranges in explicit 4/4 and refuses other
  ranges rather than rounding them. Engine enumeration uses actual clip records,
  so it does not drop stored placements beyond the visible Playlist grid.
- Source pattern names are retained when present. Referenced empty patterns can
  retain silent placements without invented events.
- Validated tail-envelope walking (`VUUU`, counted `CHAN`/`FXRN` channel
  records, `AUIO`, `UBMP`, end marker) for the observed versions. Mixer controls
  and bitmap payloads remain opaque; structural coverage is not DSP mapping.
- An observed 0x8612 `AUIO` pool with 104-byte metadata plus length-prefixed paths,
  standard PCM/IEEE format fields and provisional frame counts. The AudioTrack
  96-byte event mapping links pool indices and fractional source-tick positions
  without rounding them to integers. Every event word and source span is retained.
- Explicitly accepted **dry audio** recovery of untrimmed full-file ranges at
  natural duration. Nonzero trim fields, stretching, unknown versions/framing,
  invalid references and unsupported metadata are refused. Clip processing is
  deliberately not restored and is disclosed in preview and native description.
- Per-pool WAV relinking through the preview's Locate WAV control or CLI. Original
  media is referenced at its chosen location, not copied. RIFF/WAVE validation
  checks format, channels, rate, bits and frame count at preview; the session
  adapter rechecks format, bit depth, channels, rate and frame count before
  creating clips. These fields survive in the format-neutral preview plan.
  Changed encoding/bit depth is refused even when rate/channels/frame count match.
  Matching metadata does not prove
  content identity. Ordinary RIFF PCM/IEEE files are supported; RF64, extensible
  WAVE and other containers are currently refused by the relink validator.
- Printable drive-qualified media paths as **unverified candidates**, separate
  from structurally decoded audio sources. Relative/Unicode paths are not
  comprehensively discovered; false positives are possible. Discovery establishes
  neither clip/pad assignment, file existence nor bundled status.
- Deterministic inspection and recovery reports. Explicit source PPQ, tempo and
  meter overrides; explicit pattern selection; exact timing conversion by default.
- Engine-independent `ProjectImportPlan` and a detached ProjectDocument adapter.
  Failures do not partially change the document. Duplicate pattern slots,
  unsupported events and zero velocity are refused by default. Coincident
  supported notes are retained independently in schema 6, with a caveat that
  single-onset piano-roll edits operate on their stack together. An explicit omission policy
  excludes unsupported semantic records from playback while preserving exact
  bytes, version, source context, offset and reason in the neutral plan, report
  and native project description. Framing and timing errors in selected content
  remain fatal under that policy. Out-of-range timing
  and lossy pattern lengths are always refused.
  Layout completeness is tracked per source track. Explicit selection can omit
  damaged layouts on other tracks and opaque payloads in unselected patterns;
  those omissions remain visible in reports/native notes. A selected track with
  an incomplete pattern list is refused, including any parsed prefix. Unknown
  global PATC codes/module references still block MIDI arrangement recovery;
  audio-only recovery does not depend on the unrelated PATC stream.
  Long patterns are checked against the actual native storage boundary, never
  clamped to a short UI limit. Optional note-tick rounding is disclosed.
- Standalone adapter commit as one JUCE UndoableAction, with undo/redo checks.
- Native preview dialog, connected only to the pre-existing `.sfs` merge branch.
  The host stages both document and audio session, including new MIDI placements,
  before committing through its existing document/session snapshot undo history.
  Original SFS is never written.
- A successful host commit ends transient pattern audition and restores the song
  arrangement, including original mute states of existing WAV clips. Newly
  recovered WAVs retain their dry, unmuted state. The previous arrangement becomes
  one undo snapshot. A refused import preserves audition, selection and history.
  Pattern audition detects native notes across MIDI pitches 0–127, independently
  of the editor's displayed pitch range or the pattern's length in steps.

The product compiled and linked successfully. The actual neutral session adapter
was exercised with Tracktion: three staged tracks, 62 exact MIDI placements,
32 playable source note records, two exact preserved omitted records and three
original-file WAV clips. Complete native save/engine reopen, unchanged live input,
audio-only recovery and missing/changed media rollback passed. The preview
component's preparation/merge callbacks and offscreen rendering also passed.

The isolated Build 212 app opened Song1 through File > Merge Project, displayed
120 BPM / 96 PPQ / 4/4, previewed and merged Pro-9 A2 (seven playable notes, one
preserved unsupported record), and host Undo restored the prior blank session in
one action. A repeated import/undo/redo restored Pro-9 A2 and reported
"Redid Import SFS Content." Detached adapter undo/redo and engine save/reopen
also passed. No playback or user project overwrite was
performed. Build 213 restores the normal bundle identity after isolated validation.
The live check found that the selected pattern's button highlight remained at A1
immediately after import despite the A2 title/data; redo refreshed it correctly.
Build 214 applies that same button-refresh logic directly in the import commit.
The final refresh fix was compiled and reviewed; the live checks used Build 212.
Current app bundles were preserved; the canonical validation build is under
`build/make`. Paused Drum Rack code and the menu layout were not changed.

## Use the native preview

In Orbit, create/open a project, choose **File > Merge Project...**
and select an `.sfs`. Choose populated patterns, enter the original PPQ, tempo
and meter (96 PPQ / 4/4 are prefilled), explicitly accept tentative notes and
partial recovery, optionally enable tentative playlist recovery, then preview
or merge. Merge always validates the current settings before committing. Without that
option, no pattern arrangement is imported. To recover audio, select its Audio
row, enable tentative dry-audio recovery and locate each required WAV through
the media chooser. Preview refuses mismatched media and guessed stretching.
Recovered instrument tracks have no instrument plugin;
assign instruments manually. Drum notes do not restore drum pads or samples.
Existing projects must have matching initial tempo/meter. Source patterns with
unsupported events are refused unless their omission is explicitly enabled.
The preview lists omitted records, and their exact bytes remain in native
recovery notes. This does not establish their original playback semantics.

For Song1 the user confirmed **120 BPM, 96 PPQ, 4/4** in this chat. A1 on Pro-9 and
DrumRack converts to two tracks / 18 notes with three-bar pattern lengths. A2 on
both tracks contains a tentative zero-velocity record that Orbit's native note
setter would clamp to one; strict conversion refuses it. With explicit omission,
all A1/A2/A3 selections recover 32 playable note records and 62 placements while
preserving the two uncertain records verbatim. Each A3 is silent. The omitted
records are at offsets 3172 and 9132, both with bytes
`00000230000000003000000045000000`. Their meaning remains unresolved. Song1's
settings are not hard-coded as defaults or treated as a universal SFS timebase
mapping.

## Reproduce diagnostics and detached conversion preview

Standard-library inspection only:

```sh
clang++ -std=c++20 -Wall -Wextra -Wpedantic -Werror \
  Source/Import/OrionSfsReader.cpp Source/Import/OrionSfsReport.cpp \
  Tools/inspect-orion-sfs.cpp -o /tmp/orbit-inspect-orion-sfs
/tmp/orbit-inspect-orion-sfs '/path/to/song.sfs'
```

Optional JUCE developer tool; builds neither the Orbit product nor a native audio
session. No compiled tool is included in source deliverables:

```sh
cmake -S . -B /tmp/orbit-sfs-tool-build -DORBIT_BUILD_SFS_TOOL=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/orbit-sfs-tool-build --target OrbitSfsPreview -j 2
/tmp/orbit-sfs-tool-build/OrbitSfsPreview_artefacts/Debug/OrbitSfsPreview \
  '/path/to/Song1.sfs' --source-ppq 96 --tempo 120 --meter 4/4 \
  --accept-tentative-notes --accept-partial-recovery --pattern 0:0 --pattern 2:0 \
  --recover-tentative-playlist
```

`--pattern` uses zero-based source track and pattern indices shown in inspection.
`--recover-tentative-playlist` explicitly enables the provisional PATC mapping;
otherwise recovery remains pattern-only. `--accept-pattern-only` remains an alias
for the partial-recovery acceptance flag for older scripts.
Without selection, all populated non-audio patterns are considered and any invalid
pattern refuses the conversion. `--allow-tick-rounding` permits disclosed nearest
note-tick rounding; the adapter still refuses unrepresentable pattern lengths.
`--omit-unsupported-events` explicitly enables byte-preserving event omission;
its default is off. `--report new.txt` saves a new text report. This utility deliberately writes no
`.orbit` file: a usable native project also needs a staged audio-engine session,
which the Orbit host supplies. Writing only document XML would produce a file
that the app cannot correctly open.

## Evidence and verification

Song1 is 80,711 bytes with SHA-256
`d541b5af8cb84bf542124850af76a62715459958750eeb11ec907ab1237c8210`.
See `Reports/Song1-inspection.txt`, `Reports/Song1-pattern-recovery-preview.txt`
and `Reports/Song1-playlist-recovery-preview.txt`. Both recovery reports use the
user-confirmed timing and explicit A1-only selection. The playlist report contains
36 placements, with one-bar interruptions and three-bar source patterns preserved.
`Reports/Song1-audio-recovery-preview.txt` adds three original-file audio references:
AudioTrack 0001 at 18 seconds for 2.835 seconds, X90 - Damped at 22 seconds for
2.835 seconds and Metro at 56 seconds for 1544/44100 seconds. The original files
were found locally and their format/rate/channel/frame metadata matched the pool.
These are dry reconstructions; original clip processing remains unknown.
`Reports/Song1-complete-supported-recovery-preview.txt` selects all six referenced
note patterns, the audio lane, and explicit event omission: 32 playable note
records, 62 MIDI placements, three WAV clips and two preserved omitted events.
Song1 has 3 modules, 192 patterns, 34 tentative notes, 62 raw PATC records and 4
media candidates. Its 63,532-byte tail is now structurally walked; mixer controls and bitmap data
remain opaque, and no DSP interpretation is claimed. The reference fixture
now has all 16 track layouts recognized: 65 modules, 1,024 patterns and 110 decoded
records; all 1,124,766 bytes were retained. This is structural coverage, not full
musical conversion. Its DrumRack A1 has one coincident pair (source records at
8274 and 8290). Schema 6 preserves both records, so all-populated recovery now
validates 110 notes across three tracks with the 16 actual source placements.
The DrumRack pattern has no observed source placements; none are invented.
Conversion and native session checks use caller-supplied test timing (96 PPQ,
126 BPM, 4/4). Those test inputs
do not establish the reference song's original settings.
Neither binary fixture nor reference source is copied into this directory.

Checks run locally: both known versions, deterministic reports, byte-for-byte
reconstruction of retained source regions, malformed/truncated/oversized input
rejection, conservative policy defaults, exact 96-to-256 pattern tick conversion,
explicit selected-pattern recovery, refusal of zero velocity (and, before schema 6,
coincident notes),
atomic preparation failure, incompatible-merge rejection, maximum representable
pattern length, native document save/load and adapter undo/redo. Builds 207 and
208 linked successfully. Session tests also verified real MIDI clip ranges, saved engine
state, and engine reopen. Exact span clone/save/load, legacy behavior, referenced
empty patterns, malformed PATC refusal and beyond-grid storage were exercised.
Builds 209 and 210 linked successfully. Audio checks covered original WAV header
matching, exact fractional durations, real WaveAudioClip creation, saved media
references/engine reopen, audio-only recovery, malformed audio/pool metadata,
missing/mismatched relinks, wrong timebase and unsupported trim refusal.
The size guard was checked with sparse files at exactly 64 MiB and one byte over:
the first reaches header validation; the latter is refused before snapshot
allocation by both public reader entry points. Both real files remain readable.
Invalid tempo candidates (NaN/negative raw bits) remain opaque; explicit BPM
still works. Unsupported event-code omission preserves exact bytes, while bad
known-event timing and misaligned framing in selected content remain fatal even
with omission enabled.
Build 215 linked successfully. Actual session staging tests replaced the planned
float32 media with PCM32 and float64 files that retain the same rate, channels and
frame count. Both were refused before commit, leaving the live input and an
existing prepared output unchanged. A matching float32 replacement passed as a
control; metadata validation deliberately makes no content-identity claim.
Detached validation also refused missing encoding/bit-depth fields. Complete
mixed recovery/save/reopen and preview callback checks continued to pass.
Dependency/existing app warnings remain; no importer compile errors.
Build 216's selection checks covered an incomplete unselected Drum Rack layout,
a selected parsed-prefix refusal, an opaque A2 payload, per-track unknown pattern
references, unassignable global PATC records, audio-only independence and exact
byte reconstruction of every mutated input. The checks passed under AddressSanitizer
and UndefinedBehaviorSanitizer. Actual native staging recovered one Pro-9 lane
with 18 placements while keeping the damaged-track omission in native notes.
Build 217's extension tests verified all 16 reference track layouts, exact prefix
spans, complete raw byte retention, and refusal of wrong version/type/metadata
size/extension length/following-chunk variants under both sanitizers. The actual
preview component enabled Pro-9-only recovery while an unselected Drum Rack was
damaged, then disabled merge when the damaged parsed prefix was selected. Complete
Song1 native staging/save/reopen and prior relink checks continued to pass.
Build 218 introduced schema 6 independent note records. Strict append bounds,
stack move/resize/velocity/erase, collision/end-bound rollback, clone, schema 5
migration, future-schema refusal, save/load and adapter undo/redo passed. Reference
all-populated recovery retained 110 records and 16 placements through native engine
reopen. A separate audition-only engine check emitted 70 note-ons and 69 note-offs
for the 70-note DrumRack pattern: both coincident note-ons survive, while the
existing engine overlap policy suppresses a redundant note-off. This does not
establish equivalent Orion voice/release behavior or restore its DSP.
Actual host methods reopened/resaved every native pattern field and placement
unchanged, and stack editing/undo/redo/copy/paste retained multiplicity. An old
migration negative control changed authored data, demonstrating why the new native
pattern guard is necessary. This harness bypassed display centering because its
process has no primary display; it did not establish new desktop menu interaction.
Build 219 replaces the remaining step-based clipboard/bulk-edit intermediary with
native tick records, including pitches outside the visible editor range. Bulk
transformations retain coincident records and preserve timing fields they do not
change explicitly. Focused host checks passed with a crafted five-record pattern:
sub-step starts/durations, different-duration coincident notes, and pitches 0, 45
and 127. Exact copy/paste, bulk write, cut/undo/redo, velocity-only humanize,
start-only quantize, transpose, 100-percent and start-only scaling, velocity
scaling and randomize passthrough were checked, followed by native host save/reopen.
Modal dialog UI/setup and answers were replaced only in the temporary harness;
the production transformation bodies ran unchanged. Complete Song1 engine staging,
media revalidation/rollback and preview callbacks also passed against Build 219.
Existing app text-encoding debug assertions were observed in the headless host;
these checks do not establish a clean whole-application debug assertion run.
Build 220's actual host-method checks covered mixed-media commit into empty and
populated projects, exact native/engine inventories and ranges, exact omitted-event
provenance, one undo boundary, missing-media refusal without session/selection/history
changes, audio-only commit/undo/redo and host save/reopen. Build 219 negative controls
demonstrated two defects: a pitch-45-only pattern was considered empty, and committing
with transient audition active replaced song MIDI clips and muted new audio. Build
220 passed full-pitch 0/45/127 audition note checks at exact tick timing, created-empty
and sparse maximum-length pattern checks, and import during audition with both
originally muted and unmuted existing WAVs. Refusal preserves the saved mute records
and audition flag; successful import/undo/redo/save/reopen preserves the canonical
arrangement and original mute states. These tests invoke production host methods
with hardware disabled and display/modal setup bypassed in the temporary harness;
they are not new desktop file-picker or sound-output checks.
Ad hoc validation harnesses stayed in `/tmp`; no fixture corpus or permanent
regression suite was added.

## Still open

- Global meter/source timebase decoding and wider validation of the initial
  tempo candidate. GLOB+12 agrees with Song1's 120 and the second file's 126,
  but is not a complete versioned specification; tempo changes are unsupported.
- Wider PATC validation and unsupported stop/change codes, nonzero extra fields,
  sub-bar positions and non-4/4 arrangements. The supported observed mapping is
  tentative and opt-in. Only selected tracks need a complete assigned PATC map;
  unassignable records still block MIDI arrangement recovery. Changing native
  tempo/meter maps are refused by the session adapter when arrangement ranges
  are requested.
- Wider audio layouts, embedded/bundled audio, trimmed/stretched clips and source
  processing controls. Only the observed 0x8612 external pool and untrimmed natural
  ranges are supported tentatively. Drum samples/pad assignment/parameters remain
  unsupported; their byte-scan paths alone are insufficient evidence.
- Notes at velocity zero and other event semantics, per-note channel restoration,
  tempo/meter changes, routing and native instrument/effect behavior. Coincident
  note records are retained, but native stack editing and engine overlap handling
  do not establish the source instrument's original voice/release behavior.
- Wider corpus and resource validation. Experimental inspection/recovery refuses
  sources over 64 MiB before snapshot allocation. This is a resource policy, not
  a format limit or a strict peak-memory bound: decoded records and retained
  regions also allocate memory. Snapshot parsing avoids repeated file reads but
  is not an atomic filesystem snapshot during concurrent overwrites.
- Mixed-media file-picker/save interaction and broader GUI checks.
  Actual-window pattern import and host undo/redo are verified in Build 212;
  component callbacks, detached undo/redo, complete engine-session save/reopen
  and later headless host-method mixed-media commit/save/reopen/edit checks passed
  separately. Desktop file-picker interaction for the mixed-media case remains
  unverified.

Removal: remove this directory, both SFS tools, their CMake entries/optional target,
and the importer includes/merge callback/neutral commit bridge in Main.cpp. No
core schema or engine type depends on Orion records. The generic schema 5 exact
clip capability, schema 6 independent note list, record enumeration and pattern-name
setter can remain without the importer; removing those requires a separate native
format migration.
`OrionSfsImportModel` is the source extraction model; `ProjectImportPlan` is the neutral conversion boundary.

## Audio relink CLI

Add `--recover-dry-audio --audio-track 1` to the Song1 command, plus one
`--media poolIndex=/absolute/path/to/file.wav` for each referenced pool index:
0 = Metro, 1 = X90 - Damped, 2 = AudioTrack 0001. For audio-only recovery, omit
`--pattern` selections; explicit audio selection excludes instrument patterns.
Relinks must be supplied explicitly; no CrossOver-specific path is hard-coded.
The report preserves selected native paths and distinguishes heuristic candidates
from structured relink status. Media is never packaged with source deliverables.

The independent WAVE header validator follows the public Microsoft
[RIFF chunk documentation](https://learn.microsoft.com/en-us/windows/win32/xaudio2/resource-interchange-file-format--riff-)
and [WAVEFORMATEX field documentation](https://learn.microsoft.com/en-us/windows/win32/api/mmreg/ns-mmreg-waveformatex).
No vendor implementation was copied.
