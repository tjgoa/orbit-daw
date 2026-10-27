# Orion native effect compatibility inventory

Paste-ready reference for the Orbit project migration work. Names below were read from the user's `all native host fx.mp4` Orion Insert menu recording (2026-10-26). This is an observed menu inventory, not a claim that Orbit currently implements these effects.

## FX Dynamics
- Brickwall Limiter
- Compressor CV-82
- Compressor DNA-160
- Platinum Limiter
- Programmable Gate
- Transient Shaper

## FX Echo/Delay
- 2-Tap-Delay
- Diffuse Delay
- Filter Delay
- Multi Echo
- Simple Delay
- Stereo Delay
- Unit Delay
- X-Delay

## FX Reverb
- Chamber/Room
- Concert Hall
- Fat Space
- Large Hall
- Plate Reverb
- Impulse Response Processor

## FX Filter
- Analog Static 3-Band EQ
- Auto Tracking EQ-2
- Auto Tracking EQ-4
- Harmonic Resonator
- Low Cut
- Parametric EQ
- Remove Noise
- Sidechain Controlled Filter
- Variable Filter
- Vintage EQ

## FX Misc
- Analog Distortion AD-1
- Analog Phaser AP-6
- Chorus/Flanger
- LoFi/Noise
- Phaser
- Ring Modulator
- Rotary Speaker
- Saturation
- SSB Modulator
- Stereo Pan/Width
- Stereo Widen Pro
- Tremolo
- Tube Distortion
- Two-Band-Distortion
- Vintage Chorus

## FX Holder
- BandFX Control
- MultiFX Control
- StereoFX Control

## Compatibility work to track
- Parse Orion project effect identifiers and parameters before designing replacements; menu labels alone may not match the serialized names.
- Add a migration map from each Orion effect identifier to an Orbit native effect, a wrapper/pseudo-effect, or a clearly reported unsupported effect.
- Keep each replacement's parameter map and unit conversions beside its effect implementation.
- Compare representative settings and audio renders so replacements sound reasonably close, while documenting unavoidable differences.
- Preserve the original effect state in project metadata when no replacement can be created, so a later migration pass can recover it.
- Confirm whether the old Orion project format serializes presets, impulse responses, side-chain links, or external resource paths separately.

## Verification status
- The FX Echo/Delay, Reverb, Filter, Misc and Holder lists are legible in the supplied video frames.
- The Dynamics submenu was visible briefly but not legible in the extracted frame; its six names above need a sharper frame or a project-format inventory pass before finalizing compatibility IDs.
- No pseudo-effects are implemented by this inventory note. It records the migration scope for later implementation.
