# Epinizer 2.0.0 — 2026-10-12

- Six independently drifting ensemble voices; Density brings in the extra pairs.
- Input envelope preserves attacks and ducks the atmospheric return during notes,
  then allows it to bloom in gaps. This is envelope processing, not a spectral splitter.
- Glass: upper-octave prism layer; Massive: lower octave, centred low body and onset weight.
- Horroble: reverse slices with frequency movement; Warm: filtered tape echoes and wow.
- Hold captures a 280 ms processed loop with a 35 ms crossfade. No unity-feedback freeze.
- Linked stereo gain protection for processed output. Dry/bypass endpoints stay transparent.
- Four macros retained with their original parameter IDs; Hold appended. State schema 3
  migrates old states with Hold off. Audio stored in a captured Hold is not serialized.
- Factory presets retuned. New presets turn Echo on and release Hold.
- Existing sessions retain values and automation, but their sound changes under the new DSP.
  The previous AU installer is preserved in dist/archive/1.0.0 for returning to v1.
- New DSP character/Hold tests, realtime C++ allocation checks and equal-RMS v1/v2 demos.
