# Windows build note — 2026-10-24 (Codex)

Current cross-build directory: `build-windows-design`; package version 2.1.0.
The resource compile rule explicitly uses C preprocessing without line markers and
then `/no-preprocess` for llvm-rc. This avoids C++ `extern` fragments and octal
non-ASCII path markers being parsed a second time on the iCloud project path.
Current local toolchain: clang-cl 23.1.4 from LLVM-MinGW 20261020, used in MSVC mode.
Previous build notes below describe historical versions and are not current validation.

# Windows VST3: build and validate

## Recommended: on Windows

Install free Visual Studio 2022 Build Tools with Desktop development with C++
and CMake 3.22+. From the extracted source directory, run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-windows.ps1
```

This compiles the VST3 and tests it, then creates
`dist/Epinizer-2.0.0-Windows-x64.zip`. Only Windows VST3 is produced; no Windows AU
or macOS VST3 target exists. Run `Install-Epinizer.cmd` from the extracted ZIP.
`Validate-Epinizer.cmd` writes validation-results.txt and imposes a 60-second
limit on each test. These commands do not permanently change execution policy.

## Reproducing the cross-build from macOS

1. Extract LLVM-MinGW 20260616 macos-universal from its official GitHub release.
   Although the distribution is called LLVM-MinGW, **only its LLVM tools in
   MSVC mode** are used. JUCE rejects MinGW ABI.
2. Ensure `bin/clang-cl` points to `clang-22` (a symlink works). The directory must
   also contain lld-link, llvm-rc and llvm-lib.
3. Obtain xwin 0.10.0 and run:

```sh
xwin --accept-license --arch x86_64 --sdk-version 10.0.26100 splat --output /path/to/winsdk
```

The tested Microsoft CRT version was 14.44. Use official SDK downloads under
Microsoft's terms. The build does not purchase a licence or publish anything.

4. Set paths and build:

```sh
export EPINIZER_LLVM_ROOT=/path/to/llvm
export EPINIZER_WINSDK=/path/to/winsdk
# Optional: helper metadata generation, not a Windows runtime validation claim.
export EPINIZER_WINE='/path/to/wine'
export WINEPREFIX=/path/to/your/dedicated/wine-prefix
bash scripts/build-windows-cross.sh
```

Outputs are under `build-windows-v2-msvc`; the ZIP includes the VST3 bundle,
install scripts and five Windows test executables. CMake uses static MSVC CRT
and embeds executable manifests with LLD, so no external mt.exe is needed.

See VALIDATION.md for the current version 2 checks. The earlier version 1
passed DSP/state/direct editor and actual VST3 core checks under Wine 11.0. The hosted
editor constructed at 800×520, but its native window lifecycle timed out in
Wine; core host checks passed with `--no-editor`.
This does not establish compatibility with real Windows DAWs. Native Windows
validation is still required before treating the Windows package as release-tested.
