# Epinizer 2.0.0 — validation 2026-10-12

Host: macOS 27.0.1, Apple Silicon, AppleClang 21, CMake 4.4.4, pinned JUCE 8.0.12.
The DSP has been rewritten; these checks refer to version 2.0, not the archived 1.0 build.

## macOS AU — passed

- Universal binary: arm64 + x86_64, minimum deployment target macOS 11.0.
- Strict Apple auval: **AU VALIDATION SUCCEEDED**, native arm64 and x86_64 via Rosetta.
  Includes mono/stereo rendering, parameters, presets, scheduled/ramped automation.
- CTest: **4/4 passed** (DSP, processor/state/UI, character/Hold, realtime C++ allocations).
- Actual editor snapshot visually inspected. Four macros and Echo/Hold/bypass fit;
  callbacks/attachments update the real processor. State restores old parameters,
  migrates missing Echo/Hold, and preserves the Hold switch.
- User-level AU installation exercised with previous version backed up. The installed
  executable is checked against the final packaged executable.
- PKG built and fully expanded: its executable matches the tested AU byte-for-byte.
  pkgbuild on this macOS emitted four `write: Permission denied` diagnostics despite
  exiting successfully. The exact cause is unresolved; global PKG installation was
  not exercised. The ZIP/user installation is the tested installation route.
- ZIP checked for integrity and current documents.
- Local **ad-hoc** bundle signature only. No Developer ID, notarization or signed installer.

The system-wide PKG was not installed globally; the user-level installation was tested.
No interactive Logic session, physical Intel hardware or older macOS run was performed.
Upstream JUCE Metal code emits macOS 27 deprecation warnings; no JUCE patch was made.

## Shared DSP — passed

- Twenty 15-second impulses: 4 characters × 5 rates (8, 44.1, 48, 96, 192 kHz).
- Finite, bounded processed output, audible tail and late-tail decay; mono processing.
- Sample-for-sample block-size invariance; transparent dry and bypass endpoints.
- +18 dBFS full-wet input stress, rapid automation, character/Echo changes, smooth transitions.
- Spectral tests locate the Glass upper-octave band and Massive lower-octave band,
  verify their difference from Warm and pairwise differences among all 4 characters.
- Six-voice stereo ensemble produces measurable side content on a centred tone.
- Hold sustains a captured loop for 8 seconds without runaway gain, has bounded splice
  steps, releases to silence and clears captured audio on reset.
- AddressSanitizer + UndefinedBehaviorSanitizer pass the final DSP and character/Hold suites.
- No C++ new/new[] calls inside process during rendering and character/Echo/Hold changes.
  The release benchmark consumes the rendered samples so the work cannot be optimized away.
  About 21.33 seconds of 48 kHz stereo were rendered in 0.12 seconds on this host.
  This is a measured local throughput result, not a guarantee for other hardware or hosts.

No DTrace/malloc interception of all system code was performed. The callback has no
file access, locks or UI work. Octave layers are windowed pitch shifts with sidebands;
there is no spectral splitter, deep granular stretch or oversampled saturation.
Hold loop audio is not serialized; only its parameter state is. Restored Hold can
hold silence until the user releases it and captures new audio.

## Windows VST3 — cross-built; Wine checks passed

A real PE32+ x86-64 VST3 DLL was built on this Mac, using Clang 22.1.8 in **MSVC mode**,
Microsoft CRT 14.44 and Windows SDK 10.0.26100. JUCE was not patched; MinGW ABI was
not used. Compiler runtimes are linked statically. Import/export table inspected:
Windows system DLLs; GetPluginFactory, InitDll and ExitDll exports. JUCE's helper
produces version 2.0 moduleinfo; the package normalizes it to strict JSON.

Under Wine 11.0 on this Mac:

- Windows DSP suite: **PASS**.
- Windows character/octave/Hold suite: **PASS**.
- Windows realtime C++ allocation/throughput test: **PASS**.
- Windows processor state, migrations, direct editor construction and UI controls: **PASS**.
- Actual VST3 discovery, loading, stereo render, finite/bounded tail and state restoration
  in a separate host executable: **PASS** with `--no-editor`.

**No native Windows or Windows DAW was available.** Full hosted-window embedding,
onscreen graphics and installation/UAC scripts were not validated on Windows.
Previous v1 attempts to exercise hosted graphics under Wine timed out; v2 does not
claim that this external graphics path has been fixed. Native Windows validation
remains necessary. `Validate-Epinizer.cmd` provides five checks with 60-second timeouts.
No cloud CI or publishing was performed.

## Sound comparisons and artifacts

`dist/demos-v2/` contains original synthesized chords/drums and a short system-Milena
speech fixture. Dry, v1 and v2 use the same source; processed versions are matched
by RMS of the active section (not LUFS), subject to a peak guard. A/B files play v1,
a pause, then v2. Differences were measured on all 12 source/character pairs.
This confirms signal differences and supplies listening material; no subjective
listening panel or real singer recording was performed.

- `dist/Epinizer-2.0.0-macOS.pkg` / `.zip`: AU installers.
- `dist/Epinizer-2.0.0-Windows-x64.zip`: VST3, installers, five test executables.
- `dist/Epinizer-2.0.0-source.tar.gz`: current project + pinned JUCE and notices.
- `dist/Epinizer-2.0-interface.png`: actual current macOS UI.
- `dist/validation-v2/`: raw Apple, CTest, sanitizer, realtime, Wine and PE logs.
- `dist/SHA256SUMS.txt`: current installer/archive checksums.

Version 1 remains in `dist/archive/1.0.0`; pre-change project source is in
`backups/2026-10-12-before-v2/project.tar.gz`. Old parameter IDs are preserved,
but the rewritten DSP changes the sound of old sessions. Keep original DAW project
backups for returning to the old effect. No licence purchases or public release.
