$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
cmake -S . -B build-windows -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed' }
cmake --build build-windows --config Release --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
ctest --test-dir build-windows -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
$stage = Join-Path $PWD 'dist/Epinizer-Windows'
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item 'build-windows/Epinizer_artefacts/Release/VST3/Epinizer.vst3' $stage -Recurse -Force
Copy-Item 'scripts/install-windows.ps1','scripts/Install-Epinizer.cmd','scripts/Validate-Epinizer.cmd','scripts/validate-windows.ps1' $stage -Force
$validation = Join-Path $stage 'validation'
New-Item -ItemType Directory -Force $validation | Out-Null
Copy-Item 'build-windows/Release/epinizer_dsp_tests.exe','build-windows/Release/epinizer_character_tests.exe','build-windows/Release/epinizer_realtime_tests.exe' $validation -Force
Copy-Item 'build-windows/epinizer_plugin_tests_artefacts/Release/epinizer_plugin_tests.exe' $validation -Force
Copy-Item 'build-windows/epinizer_vst3_host_tests_artefacts/Release/epinizer_vst3_host_tests.exe' $validation -Force
Copy-Item 'README.md','THIRD_PARTY.md','VALIDATION.md','CHANGELOG.md' $stage -Force
Copy-Item 'План следующей версии.md' $stage -Force
New-Item -ItemType Directory -Force (Join-Path $stage 'Assets') | Out-Null
Copy-Item 'Assets/README.md' (Join-Path $stage 'Assets') -Force
Compress-Archive -Path $stage -DestinationPath 'dist/Epinizer-3.0.0-Windows-x64.zip' -Force
