# Orbit development progress

## 2026-10-26 — Build 66 / v0.3.45

Orbit has moved from a basic DAW shell toward the generator-first workflow that guides the project. This build compiles locally and adds several of the editing behaviors shown in the Orion references.

### Newly implemented

- Keep instrument creation focused: inserting a generator no longer opens the Mixer. The Mixer stays embedded in the main workspace, and clicking a generator or Mixer surface brings it forward.
- Make the Piano Roll settings easier to reach with a small keyboard icon and options for grid spacing, emphasis, velocity lane visibility, colored keys, and pitch labels.
- Improve Playlist editing with right-click clip deletion, a pattern menu anchored beside the clicked clip, subtle MIDI note previews, and lighter grid lines.
- Continue the VST3 workflow with standard-folder scanning, configurable paths, scan progress, and local diagnostic reports.

### Current focus

Orbit remains in active development. The next step is hands-on verification of the panel layering and Playlist interactions, followed by continued work on plugin-state recovery and the remaining project workflow. Build 66 has been compiled, but this update has not yet had a full manual UI/audio verification pass.

See [`BUILD_HISTORY.md`](BUILD_HISTORY.md) for the full build timeline and [`ORION_LEARN_TODO.md`](ORION_LEARN_TODO.md) for the working backlog.
