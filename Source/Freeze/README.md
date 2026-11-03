# Song Freeze and permanent audio conversion

Open a generator's menu (or right-click its Playlist track) and choose **Freeze
Song…**. The first version prints that generator's Song arrangement, starting at
Song time zero and ending after its last placed pattern plus the release/FX tail.
It preserves leading silence. It does not render a per-pattern cache bank.

The options are:

- **Reversible Freeze** (default): original patterns, arrangement, automation,
  generator state and original bypass flags stay in the project. **Unfreeze**
  restores them. Both actions are one Orbit Undo/Redo transaction. The mixer
  fader, pan, sends, outputs and Master continue to work normally.
- **Bake mixer inserts and EQ** (off by default): print the selected track's
  ordered processing before the fader, including its isolated Multi FX rack if
  present. These processors are bypassed/omitted during frozen Song playback and
  protected from editing. Their original settings return on Unfreeze. With this
  option off, only the generator and its MIDI processor are printed; all mixer
  processing remains live. Shared bus/return and Master effects are always live.
- **Keep live Pattern audition** (on by default, reversible only): Pattern Play
  temporarily runs the preserved generator and any baked processors with their
  original bypass flags. Song playback uses the WAV. With this option off the
  frozen track is silent in Pattern mode. Pattern selection is available in the
  read-only generator window; Unfreeze before editing its sound or notes.
- **Release / FX tail**: minimum extra seconds after the last placed pattern.
  Default 10, adjustable 0–600. Finite reported tails of enabled serial processors
  are also allowed for, including isolated rack members. Plugins reporting an
  infinite/unknown tail use the chosen manual allowance. Increase it for unusually
  long internal releases/feedback. Mixer tails remain live unless their effects
  are baked.
- **Permanent Audio — discard source / Undo**: explicitly destructive conversion.
  Mixer inserts/EQ are baked and live Pattern audition is disabled. Only after a
  complete verified WAV is available, the generator, its MIDI processor, printed
  effects/isolated rack, original patterns/placements and their source automation
  are removed. The same lane becomes an ordinary audio track. **Orbit and native
  Edit Undo histories are cleared**, so this action cannot be Unfrozen or Undone.
  Live fader/pan/sends/output and unprinted automation remain. Existing project
  files/backups are not deleted or overwritten until the user saves normally.

Rendering uses bounded native render batches between message-loop turns, so
Cancel and graph cleanup run on the correct thread without a worker-join
deadlock. The console harness drives these same batches directly; it does not
construct a native progress alert because it has no display.

Rendering uses an isolated Edit containing only the required processors, not all
other VST instances. A cancel, disk error, incomplete file or changed source cannot
commit a partial freeze/conversion. Source DSP is omitted from the live Song
playback graph; reversible Freeze preserves its instances/state for audition and
Unfreeze. Use Permanent Audio when the editable source should be discarded.

Source edits and tempo/meter changes require Unfreeze. Musical signatures also
catch less common import paths that change frozen source data; those recover the
original rather than play stale audio. Missing/unreadable caches restore the
original generator and show a recovery message. Permanent audio has no source to
recover; ordinary missing-media handling applies.

Caches are stereo 32-bit floating point WAVs at the current device sample rate:

- Saved project: `<project name> Audio/Freeze/<track ID>-<render ID>/generator.wav`
- Untitled: `~/Music/Orbit/Freeze/<project ID>/<track ID>-<render ID>/generator.wav`

Unfreeze and Undo retain WAVs for Redo and older saved versions. Save As retains
existing media references, as with recorded audio; it is not a collect-and-copy
operation. Carry the referenced Audio folder when moving a project. Marked cached
WAVs use Tracktion's real-time reader to avoid the legacy proxy reader's two-sample
interpolation delay; this setup is reapplied on load/Undo and native rendering.

The initial implementation refuses MIDI routing between generators and synth
sidechains. Mixer baking additionally refuses sidechains, shared/nested racks,
and chains where a fader/send/return precedes processing that would be printed.
It never silently reorders these chains; keep mixer processing live or rearrange
that routing first. Unfreeze a generator before merging it into another project;
cross-project frozen-source identity remapping is not part of this version. Multi-output stems, region/selection bounces and individually
rendered Pattern banks are later features.

Schema 25 is required so older Orbit versions reject these projects safely.
`Tools/test-freeze-host.cpp` exercises the native renderer, waveform alignment,
Song/Pattern isolation, original bypass/parameters/rack restore, live mixer edits,
Undo/Redo, persistence/recovery, failures/cancel and permanent conversion. Native
VST host/hardware behavior should also be checked with the user's chosen plugins.

## Editable frozen audio (Build 323)

Reversible Freeze now displays its real Wave clip in the Playlist. Move, copy,
split, trim, reverse, normalize and delete this audio using normal audio actions.
Audio edits survive refresh, Pattern/Song switching, Undo/Redo and save/reopen.
Deleting the complete print leaves an intentionally empty frozen lane; it is not
silently regenerated. Right-click **Show Ghost Notes** optionally overlays the
original MIDI faintly on the waveform, mapped through source offsets/reversal.

The original patterns remain preserved separately. **Unfreeze Generator** discards
that lane's edited frozen-audio arrangement and restores the original patterns.
Undo Unfreeze restores the edited audio arrangement. Copies placed on ordinary
audio tracks remain independent. In Pattern mode the frozen Wave clips stay in
the project but are temporarily muted, while optional live source audition runs.
This is still a Song render, not separate rendered Pattern banks.
