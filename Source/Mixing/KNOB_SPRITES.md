# Live mixer knob sprites

Open the Mixer or Master header menu and choose **Knob Sprites...**. Select the
KnobMan export folder containing `manifest.json` and `sprites/`. Exported PNGs
are decoded on a background worker; rapid clicks discard stale results.

Use **Artwork: Unknown / Unipolar / Bipolar** to classify a selected sprite.
Explicit manifest polarity metadata is supported; otherwise the label starts at
Unknown. This describes artwork, separately from the actual control parameter.
Unknown sprites preview without replacing controls in grouped mode.

Selecting a classified sprite applies it to the matching group automatically:

- **Bipolar controls:** EQ gain and pan, with zero/center at the middle frame.
- **Unipolar controls:** sends, frequency and Q, using their existing normalized
  minimum-to-maximum parameter mapping.

Optional role overrides select a distinct send, EQ gain, frequency/Q or pan
sprite. A mismatched artwork classification cannot replace that role. **Use
normal knobs** clears the chosen override, falling back to its group or the
native skin artwork. None of these choices changes audio values or automation.

For unrestricted audition, check **Test one sprite on all controls (temporary)**.
Any selected sprite, including Unknown artwork, appears on every mixer knob.
Saved group choices remain intact. Uncheck to restore them. This test mode is not
restored on application startup.

**Star favorite** and **Favorites only** help make a shortlist. A polarity filter
narrows the library. **Remove from list** hides an entry without deleting its PNG
or editable source; use **Show removed** and **Restore to list** to recover it.
Classifications, favorites and removals are stored per library and stable sprite ID
in local application preferences. No source asset is modified.

The **Knob size** slider previews 100–500% immediately in the real mixer. At 500%
the imported artwork fills the available control cell. Strip width, row spacing
and mixer window bounds do not change. For more room, use **Mixer Layout...**.
Common transparent padding is cropped across all frames together, so the artwork
stays centered as it turns. Original PNGs and soft shadows are preserved.

Folder/sprite choices restore across Mixer/Master banks and skins. Missing or
invalid PNGs report an error and retain the previous working selection. Resource
limits and path checks reject malformed manifests and incompatible strips.

A future optional right-click KnobMan editor/export/reload workflow is planned;
the editor is not embedded in this version.
