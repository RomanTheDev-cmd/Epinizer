# Epinizer 2.1.0 — 2026-10-24 (Codex)

- Refined controls: borderless buttons/values, thin dial arcs and soft dial shadows.
- Fixed local PKG ownership diagnostics, retaining root:wheel archive/BOM ownership;
  payload and BOM are regression-checked against pkgbuild's native reference.

- Developer displayed as MyDarknessDays; existing plugin codes and bundle ID retained.
- Original dark castle/overgrown entrance artwork, monochrome controls and forest-green gradients.
- New 1000×640 editor with five real dials: Space, Density, Motion, Width, Mix.
- Width narrows wet stereo only; 100% preserves the previous sound. Mix 0 / bypass are unchanged.
- State schema 4 reads older sessions with Width=100%; original parameter IDs/order retained,
  Width appended. Its setting is saved and automated, and remains unchanged by factory presets.
- The v2 ensemble/reverb/echo/Hold algorithms otherwise remain in place. The planned
  combined-effect sound redesign has not been implemented in this visual release.
- Visual provenance and the generation prompt are recorded in Assets/README.md.

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
