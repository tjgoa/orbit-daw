# Playlist / Recording implementation checklist

Reference: the complete installed Orion Playlist.htm and configuration image,
reviewed in ORION_MANUAL_AUDIT.md. Manual assets remain reference material only.
Work through this chapter before advancing through Automation. The user explicitly requested the missing Song Event Editor in Build 254, so that usability fix proceeds alongside this checklist. Sampler RNG
was explicitly deferred by the user until the Sampler chapter.

## Completed through Build 251

- Source-duration Draw strokes, intentional right-edge placement extensions,
  opaque multi-bar clips and collision protection.
- Wheel and held vertical pattern choice, blank/existing-slot policy, remembered
  generator slot, one gesture Undo.
- Continuous floating title dragging, ruler click/loop distinction, maximized
  workspace coverage and resizable Generator column.
- Arrangement rectangle/additive selection, group move/copy/delete, one Undo,
  atomic validation, cross-generator complete pattern cloning.
- Native audio moves/copies preserve offsets and clip state; missing media works.
- Focused hover-N rename and Shift+S follow; saved Follow, note/velocity previews
  and horizontal navigation visibility.
- Existing MIDI take recording, sustain, quantization, pinned destinations and
  atomic commit/reopen (limits in Source/Recording/README.md).

- Build 251: saved metronome quarter-note count-in, input exclusion/cancel,
  completed-note Pattern loop replay before commit, common Pattern MIDI cycle,
  one take Undo and native persistence.

## Still required in this chapter

1. Step/free placement and temporary Shift/Alt snap behavior. Native pattern
   arrangement positions currently use integer bars; settings alone are insufficient.
2. Generator/Mixer Song event ranges moved/copied with clips, explicit destination
   target mapping and safe shared Undo. Currently Song lanes stay in arrangement
   time; Pattern automation already copies with its pattern.
3. Separate Song recording of live pattern changes, rather than recording MIDI
   notes into fresh patterns only.
4. Audio recording, per-track hardware input and monitoring, multichannel capture,
   media commit/recovery and hardware validation.
5. Freeze rendering and multitimbral channel expansion with shared instrument
   routing (cloning a plugin is not channel expansion).

Automation follow-ups through Build 255 now include the separate Song Event
Editor, selected per-control entry points, curve tools, arranged Pattern curves
and native parameter takes. Tempo and remaining lane-identity/tool gaps are
tracked in Source/Automation/IMPLEMENTATION.md.

## Validation for Build 249

Build 251 recording validation is documented in Source/Recording/README.md.

### Playlist checks

Tools/test-options-host.py --sanitize --playlist exercises production cell and
Grid handlers, complete group transactions, rejected collisions/slot exhaustion,
integer bounds, cross-generator Pattern automation/Accent/Slide/coincident notes,
native audio offset/reversal/missing-media state, save/reopen, dialog target
identity, view rendering, follow navigation and fresh-process preferences. Both
navy/light themes are rendered for inspection. Hardware is disabled; headless
engine diagnostics are not physical audio/MIDI or all-plugin certification.
