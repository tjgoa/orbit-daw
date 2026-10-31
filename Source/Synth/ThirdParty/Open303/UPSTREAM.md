# Open303 DSP source

Upstream: https://github.com/maddanio/open303
Pinned revision: 52f8614966cde2c1fd9d76369cf9d07e766262ed
Original author: Robin Schmidt, 2009, https://www.rs-met.com/
Licence: MIT; the complete upstream notice is in License.txt.

Only Source/DSPCode and the licence were brought into Orbit. The old VST2
wrapper, SDK, upstream builds and binaries are not included or required.
The included FFT is by Takuya Ooura; see OOURA-LICENSE.txt.

Local portability/safety changes, 2026-10-29:

- Normalize line endings to LF.
- Use a fixed 32-bit UINT32 on LP64 systems and std::bit_cast for exponent
  extraction, avoiding pointer aliasing and width assumptions.
- Restrict old MSVC assembly rounding to 32-bit x86; use the existing portable
  fallback on x64. Use standard sin/cos on every platform.
- Clear prototypeTable only to its declared tableLength. The upstream loop
  cleared four elements beyond the allocation; UBSan exposed this during tests.

Orbit's voice adapter calls the core's trigger/slide routines from fixed-capacity
note state, bypassing the upstream std::list MIDI allocator and internal
sequencer. Wave tables and FFT resources are initialized outside note rendering.
The 303-specific oscillator/filter/envelope DSP remains upstream code. Orbit's
UI, native step-switch storage, MIDI markers and engine integration are separate.

Behaviour references reviewed:
https://www.firstpr.com.au/rwi/dfish/303-unique.html
https://articles.roland.com/mastering-the-tb-303-sequencer-in-roland-cloud/

This is credited MIT source reuse, not a claim of a strict clean-room process or
sample-identical Orion/physical TB-303 reproduction.
