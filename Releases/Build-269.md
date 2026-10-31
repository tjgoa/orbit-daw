# Orbit Build 269 / v0.3.248 — local development update

- Independent FX panels open immediately on insertion. Keep multiple effects,
  generators, Playlist, Piano Roll and Event Editor open together.
- FX minus minimizes to the bottom bar; click its tab to restore. FX X removes
  that plugin, with Undo support. Existing panel placement/minimized state is
  retained through unrelated Undo operations.
- External VST instruments also get independent generator panels.
- New FX returns start empty. Right-click a send knob to choose built-in/VST3
  effects directly. Saved projects keep their existing effects.
- Mixer EQ/send knob arcs and pointers now agree across their complete range.

Validated with native sanitizer profiles and actual RipplerX/QDelay instances,
including a native-peer test with two instrument GUIs and two FX GUIs together.
No DSP-range or project-schema change. No public release was published.

Testing instructions: Source/Editing/TESTING_GUIDE.md, Build 269 section.
