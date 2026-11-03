# Multi FX Rack — Build 305

Choose **Insert > Multi FX Rack**, or choose it in a channel, Master or FX-return
insert menu. One rack occupies one insert slot. A new rack is empty and passes
stereo audio through; no default effects are loaded.

Use **Add effect** for Compressor, Reverb, Delay, Chorus, Phaser, Low-pass Filter,
Look-ahead Brickwall Limiter, Peak Compressor, Tube Distortion and Two-Band
Distortion. Already scanned VST3 audio effects appear in the same list. Scan them
through Orbit's existing VST3 Effects browser before opening a new rack editor.
Synths and nested racks are excluded from this serial audio-effects container.

The numbered chain selector chooses the effect whose controls appear below it.
**Enabled** bypasses that slot; **Up / Down** change its position; **Remove**
deletes that slot. The path summary shows the chain order. VST3 effects can use
**Plugin GUI** for their existing native editor, or the host parameter controls.
Resize the rack window or scroll when an editor needs more space.

The rack window's minus minimizes the complete editor and docks it. Its X removes
the rack insert, following Orbit's existing FX-window convention. Editing or
opening a rack does not close other windows. Adding effects preserves transport
playback. Structural edits follow the existing Undo policy: an active automation
take is committed before creating the rack; this is not a new recording mode.

Rack contents and order persist in the project. Copy Effect Settings / Copy
Insert Chain and paste create separate rack types and separate effect IDs.
Changing the pasted chain does not change its source. Child Song automation is
discovered by instance ID; unique name/parameter pairs can use Pattern events.
Effect settings copy excludes automation and external child sidechain routing.
The initial rack panel does not add a new external-sidechain routing UI.

Project Merge copies each imported rack definition and remaps the rack reference
and child Song-automation IDs alongside the imported track. The merge remains
one Undo/Redo operation.

The audio graph, parameter automation and latency handling use Tracktion's native
RackType / RackInstance implementation. Open and merge pass detached engine
ValueTrees so Tracktion can identify live rack instances under their Edit root.
No changes to Tracktion's vendor source are required.

Native checks cover all ten built-in effects in one audible chain, per-slot
reorder/remove/bypass, independent chain copying, one Undo/Redo, Song parameter
discovery, active playback during insertion, and audio after a fresh-process
project reopen, plus project merge audio/Undo/Redo and child automation remapping. The scanned VST3 path reuses the existing host/editor wrapper;
individual third-party VST3 effects and their nested native GUI presentation are
not newly certified by these native built-in tests.
