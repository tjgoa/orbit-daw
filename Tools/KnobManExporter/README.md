# Orbit KnobMan Exporter 0.2.0

Exports editable `.knob` projects through **g200kg’s actual WebKnobMan renderer**.
The gallery displays those PNG frames, not HTML/CSS approximations. This tool
covers handover milestones A/B. Native Orbit mixer audition and the layout editor are available in the current source.

## Start on this Mac

Double-click **Export Knobs.command**, choose your `.knob` source folder, then
choose a separate output folder. Terminal shows per-project progress. At the end,
the local, self-contained `index.html` gallery opens. You can also open it later
by double-clicking it. Use its sliders or Animate button, and compare Silver,
White, Navy and transparency backgrounds. Large images decode as cards come into
view. No local server is required to view the gallery.

Requirements: macOS, Python 3.10+, Git and installed Google Chrome. First launch
creates `.runtime/venv` beside the tool, installs the pinned Python packages and
downloads a pinned official WebKnobMan checkout into `.runtime/webknobman`.
Internet is required for that setup; projects render locally. The renderer’s
browser context blocks external requests, including uploads, ads and web fonts.
It uses an isolated headless Chrome session, leaving your open browser alone.

Your source projects are read only. Existing exported sprites are validated
before reuse. Changed settings produce separate content-addressed PNGs. Failed
projects appear with an explanation, and completed items survive interruption.
Rerunning a subset preserves the other manifest entries. Do not run two exports
into the same output folder at the same time; a process lock enforces this.

## Command line and overrides

```sh
python3 -m venv .runtime/venv
.runtime/venv/bin/python -m pip install -r requirements.txt
.runtime/venv/bin/python export_knobs.py /path/to/projects -o /path/to/library
```

Multiple source files/folders are accepted; folders are searched recursively.
ZIP input is not implemented. An export directory can contain the following:

- `sprites/*.png`: static transparent sprite sheets, using original frame settings.
- `sprites/*.json`: normalized dimensions, orientation, provenance and hashes.
- `manifest.json`: library entries, failures, renderer version and source references.
- `index.html`: offline scrub/animation gallery with the exact exported pixels.

Options:

```sh
# One source and a five-item compatibility pass
.runtime/venv/bin/python export_knobs.py /path/to/one.knob -o /path/to/check
.runtime/venv/bin/python export_knobs.py /path/to/projects -o /path/to/check --limit 5

# Overrides affect export only, never the source file
.runtime/venv/bin/python export_knobs.py /path/to/projects -o /path/to/library --size 96 --frames 101 --orientation vertical

# Explicitly allow switches/faders with intentionally transparent states
.runtime/venv/bin/python export_knobs.py /path/to/switch.knob -o /path/to/library --allow-blank-frames

# Force rerender rather than validated reuse
.runtime/venv/bin/python export_knobs.py /path/to/projects -o /path/to/library --retry
```

Other options: `--choose`, `--open`, `--timeout 360`, `--browser chromium`
(after `python -m playwright install chromium`), `--renderer /path/to/pinned/checkout`.
The default per-project timeout is 240 seconds. Input is limited to 32 MiB;
output defaults to 32 million pixels and a maximum 32767-pixel sheet dimension.
Use smaller size/frame overrides for very tall strips. Oversized, missing-layer,
unreadable-image, all-transparent and partially blank outputs are rejected.
`--allow-blank-frames` permits individual empty states with a recorded warning;
it never permits an entirely blank sprite. Static repeated frames are warned,
not rejected. These checks catch common partial renders, but visual review is
still needed to judge an unfamiliar source’s intended appearance.

Horizontal output rearranges the original vertical frames without resampling;
this avoids the pinned upstream Export function’s horizontal canvas allocation
bug. The engine’s layer rendering and effects are unchanged. Only its render
timer pacing is accelerated. No image-generation model is involved.

## Provenance and redistribution

Renderer: https://github.com/g200kg/webknobman

Pinned revision: `c3791b631fb663f899850b1dbff9b2426296851e`.

The checkout must be clean and at that revision. Upstream is MIT licensed;
its notice is retained in `WEBKNOBMAN-LICENSE.txt`. The renderer is fetched, not
bundled. This wrapper is part of Orbit’s AGPLv3 source; see `LICENSE.txt`.
Individual knob designs retain their own authorship and license. A renderer
license does not license the designs. Source references/hashes and available
credit/license fields are kept. No user knob collection is bundled in the tool.
