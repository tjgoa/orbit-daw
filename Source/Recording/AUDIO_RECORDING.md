# Audio input recording — Build 315

Audio is a separate record-content mode. Song Freeze/Unfreeze is documented in
[the Freeze guide](../Freeze/README.md).
This uses Tracktion's native input capture, latency compensation, background disk
writer and count-in, not a GUI timer or a copy of the output mix.

## Record a take

1. **Insert > Audio Track > Add Audio Track**. The first audio lane selects Song
   mode automatically. Song transport can run on an empty timeline; no generator
   or existing clip is required.
2. **Options > Audio Input Settings**: choose the input device/channels, enable
   audio input, choose mono channels or stereo pairs, bit depth and monitoring.
   Monitoring defaults off. Use headphones or interface direct monitoring where
   appropriate. macOS requests microphone access when its input is opened.
3. Right-click the audio track and choose **Recording Input…**. Select one mono
   channel or stereo pair for that track. When a fresh track is armed and there
   is exactly one already-enabled source, it is assigned automatically and named
   in the status bar. Other tracks may use other inputs or the same source.
   Explicit assignments, including missing device IDs, are never replaced
   automatically. Device/mono-stereo changes may therefore require reassignment.
4. Arm the track's **R** button (this selects **AUDIO** and, while stopped,
   **Song**), turn Loop off and position the cursor in an empty region. With no armed audio
   tracks, AUDIO records the selected audio track. Instrument R arms are ignored
   in AUDIO mode; MIDI, AUTO and CLIPS do not record hardware audio.
5. **Record**, then **Play** or Space. Record while playing punches in immediately.
   Recording Settings' count-in applies to a stopped start. The Playlist shows a
   growing live waveform; the completed audio clip remains after Stop.
6. **Stop/Space** finishes the take. Pressing Record during capture finishes it
   while playback continues. Stop during count-in cancels and restores the start
   cursor without adding clips or an Undo action.

The top Record indicator stays on during native audio capture and the track's
R remains armed after Stop. If Play cannot begin a take (for example, input is
disabled or unassigned), the stopped Record arm remains active and the status
bar explains the problem. Correct setup and press Play again; Record or Stop
cancels the pending arm. Record indicator updates include AUDIO as well as MIDI,
AUTO and CLIPS.

All armed audio tracks are one take/Undo. Input captures before track inserts,
EQ and faders; monitoring/playback use the existing track processing. Simultaneous
mono inputs remain separate; stereo pairs remain interleaved stereo WAV files.
Existing clips are protected: occupied start positions are refused, and the
common capture range ends at the first later clip on any destination or the
512-bar Playlist boundary. Even if the GUI timer runs late, native commit is
clamped to that range. A device disconnect stops the take and reports the issue.
Explicit Save and project replacement finish capture first; autosave waits.

## Media and recovery

For a saved project, files go to `<project name> Audio/Recordings/Take-<UUID>`
beside the project. Untitled projects use `~/Music/Orbit/Recordings/<project ID>`.
**Recording Input > Open Recordings Folder** opens that project's recording root.

Each take folder has its WAV files, a `take.json` manifest and `before-take.orbit`.
After a successful commit, `recorded-take.orbit` is also saved there, so a take
can be opened even if the main project has not been saved again. Undo/clip deletion
does not delete WAVs, preserving Redo and previous project versions. Save As
retains references to the existing media; keep those folders when moving projects.
This is not a collect-and-copy-media command.

Disk/commit errors retain available files and mark the manifest `needs-review`.
Completed WAV files can be imported manually. This does not promise automatic
repair of incomplete WAV headers or automatic restoration of a recording after
power loss/process termination. The manifest is diagnostic evidence, not a
sample-by-sample crash journal.

## Scope and validation

This first pass requires 4/4 Song mode and Loop off, matching the existing
Playlist pattern recording restrictions. General meter, loop takes/comping,
automatic punch regions, destructive replace, internal resampling, automatic
crash salvage remain separate work. Song Freeze is documented in ../Freeze/README.md. Hardware-specific latency and
macOS microphone permission prompts need a physical-device check.

`python3 Tools/test-options-host.py --sanitize --test-file Tools/test-audio-recording-host.cpp`
feeds actual hosted audio callbacks, without opening hardware. It verifies known
signal samples/channel identity, 16/24-bit mono/stereo WAVs at 48 kHz, simultaneous
tracks, count-in exclusion/cancellation, punch-out, live preview, monitoring
on/off, future-clip protection despite timer overshoot, atomic Undo/Redo,
media retention, input selector state, native engine stop, recovery-file reopen,
preservation of live mixer changes across take Undo and native project save/reopen.

`Tools/test-audio-recording-workflow-host.cpp` adds the empty/audio-only Song
workflow through enabled production R/Record/Play callbacks. It includes regular
host timer updates, visible arm state and Playlist playhead, failed-start retry,
single-source assignment without replacing explicit IDs, exact WAV sample/channel
checks, subsequent Song playback, Undo/Redo and project reopen. These use known
hosted input samples and do not open a physical microphone. The console harness
does not run NSApplication's posted button event loop; it invokes the same
enabled/toggle state and production callbacks directly.

## Build 323 live capture waveform

The pending Playlist take draws the native RecordingThumbnailManager thumbnail
as sample blocks are written, with a small REC label. Host timer repaints keep the
waveform growing alongside the transport. The waveform test checks thumbnail
length and a known captured peak before Stop, then checks committed WAV samples,
playback, Undo/Redo and reopen. The user confirmed physical microphone capture
before this iteration; hardware capture is not reopened by the console test.

Audio clip Normalize now measures only the played source excerpt and applies
non-destructive gain, with Undo. See ../Playlist/AUDIO_EDITING.md for audio and
frozen-track placement tools.
