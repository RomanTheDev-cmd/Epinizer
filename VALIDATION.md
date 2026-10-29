# Epinizer 2.1.0 — 2026-10-24 (Codex) Validation

## Scope and evidence

2026-10-24 polish: permanent outlines removed from buttons and percentage fields;
soft dial shadows retained. Fresh universal macOS and Windows builds passed;
universal CTest 4/4 and Wine state/direct-editor checks passed. The new actual JUCE
snapshot and a second scoped static finish review found no blocking clipping/overlap.
Interactive keyboard and DAW use remain untested.

MyDarknessDays branding, original castle interface and wet Width. The larger
combined-effect sound specification remains unimplemented. PowerKick and PowerBass
were read only as references; no project files were changed or built there.

- Native arm64 CTest: 4/4 passed; universal build CTest: 4/4 passed.
- Universal AU contains arm64 + x86_64. Rosetta DSP and state checks passed.
- Width values 0 / 0.37 / 1 tested in all four characters against independent
  wet mid/side expectations at quiet levels; dry and bypass remain unchanged.
- Saved pre-redesign DSP compared with Width=100%: sample-for-sample equality
  for four character renders. This is a bounded numerical comparison, not listening.
- Width state round trip, migration from schema 1 and 3, five slider attachments,
  character/preset/button callbacks and bounds of visible controls passed.
- Actual native JUCE PNG at 2x visually inspected. Independent static finish review
  found no blocking overlap/clipping. This is not an interactive DAW GUI test.
- Developer verified in AU Info.plist and VST3 moduleinfo as MyDarknessDays.
  Since 3.0.1 (2026-10-29): series identity Mddy/Epnz and com.mydarknessdays.epinizer (was Epna/Epnz,
  audio.epinizer.effect of the old "Epinizer Audio"); auval lists it as `aufx Epnz Mddy - MyDarknessDays: Epinizer`.
- User AU installed with the previous user bundle backed up. Installed executable
  SHA-256 is checked against the signed package, not the bare iCloud build folder. No PowerKick/PowerBass installation changed.
- Apple auval strict passed arm64 and Rosetta **for version 2.1.0**, vendor
  MyDarknessDays. Initial runs still loaded cached 2.0 and are separately archived;
  only the subsequent explicitly identified 2.1 runs count for this release.
- Windows PE x64 built with clang-cl 23.1.4/MSVC ABI and static runtime.
  Wine: DSP, character/Hold, realtime allocations, state/direct editor and actual
  VST3 discover/load/render/state (`--no-editor`) passed; manifest emitted correctly.
  The path-sensitive host test passed with an ASCII Wine drive alias. Its initial
  raw Unicode argv path failed discovery; that failure is retained in the logs.
- Windows RC build initially failed on escaped Cyrillic line markers. Resource
  preprocessing now explicitly uses C without line markers and avoids a second pass;
  a focused resource check and the complete Windows build then passed.

## Known limitations

Wine raster snapshot timed out after 60 seconds; Windows visual/hosted GUI validation
is not claimed. No native Windows DAW, installer/UAC, physical Intel Mac, other macOS
version, interactive Logic editing, keyboard/screen-reader or musical listening test
was performed. The new larger DSP architecture is still a proposal.

Mac packaging initially failed signing the live iCloud build because FinderInfo/file-provider
attributes were attached to the bundle. Packaging now copies without metadata into
a clean staging directory and signs/verifies that copy. The user installer also copies
without metadata. The original cloud metadata is not removed from the workspace.

The common JUCE code still emits macOS SDK deprecation warnings. No JUCE source was
patched. Ad-hoc AU signing only: no Developer ID, notarization or signed installer.
System PKG installation remains untested; the user-level ZIP route was exercised.
The original pkgbuild ownership mapping emitted four `write: Permission denied`
diagnostics, also reproduced with a one-file payload outside iCloud. Packaging now
uses `--ownership preserve`, then explicitly writes root:wheel archive/BOM ownership
before flattening. Stderr is not filtered. The new packaging run has none of those
diagnostics. Two packaging regressions passed: a complete decompressed payload and
BOM comparison with Apple's recommended-ownership reference (including executable
modes, nested directories and spaces), and rejection of invalid/truncated payloads.
Only the current gzip/odc pkgbuild format is supported; an unexpected format or BOM
round-trip mismatch aborts packaging. This does not prove successful global installation.
The old system-wide 1.0 AU remains installed; choose
MyDarknessDays / Epinizer to use the updated user-level plugin.

The background was generated from text without a reference image; controls/layout are
implemented for Epinizer, system fonts are not bundled. Provenance is Assets/README.md.
No legal clearance or guarantee of universal non-infringement/trademark availability
was performed; existing JUCE/dependency licence obligations remain.

Raw logs and exit codes: `dist/validation-ui-20261024/`. Rendered design evidence:
`Дизайн/2026-10-24/`. Documentation-only changes after the compiled checks do not
change the tested binaries. Package integrity/hash checks are recorded separately.

---

## Historical 2.0 results (not a fresh check of 2.1)

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
