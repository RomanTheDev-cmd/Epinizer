# Epinizer

## Platform
Native desktop audio plugin, C++17 / JUCE. AU on macOS; VST3 on Windows.

## Purpose and operating context
Roman's combined atmosphere effect for instruments, voice and drums in a DAW.
Four characters: Glass, Massive, Horroble, Warm. Keep the exact name Horroble.
Simple direct manipulation; the user requested four sound controls plus a fifth Mix.
The larger sound redesign is specified separately in «План следующей версии.md»;
its proposed algorithms must not be presented as already implemented.

## Brand commitments — Roman, 2026-10-24
Developer name: MyDarknessDays. Product: Epinizer.
Create an independent design. Reference the general clarity of PowerBass/PowerKick,
with monochrome controls, green gradients, gloomy abandoned castle entrance and
abundant vegetation. Do not copy their artwork or edit either project.

## Functional boundaries
Version 3.0 (2026-10-28): the sound is rebuilt as four separate chains (see README); the first three dials are
the character's own macros on the same parameter IDs (space/density/motion), plus wet Width and Mix.
Preserve factory presets, character selection, Echo/Hold/bypass, automation and saved state.
No extra pages or ornamental controls. Width does not alter dry or create a second
output on a mono bus. The 2.x sound (ensemble, octave layers, shared room) is gone; acceptance is Roman's listening in a DAW.

## Evidence and limits
Existing source, tests and local packages are available. Fresh validation and rendered
screenshots belong to each build. Tests and pictures do not prove subjective sound
quality, a native Windows DAW run or freedom from every intellectual-property claim.
