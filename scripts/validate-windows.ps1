$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$resultFile = Join-Path $PSScriptRoot 'validation-results.txt'
'Epinizer Windows validation — ' + (Get-Date -Format o) | Set-Content $resultFile
$cases = @(
  @{ Name = 'DSP'; Exe = 'epinizer_dsp_tests.exe'; Args = '' },
  @{ Name = 'Character engines and Hold'; Exe = 'epinizer_character_tests.exe'; Args = '' },
  @{ Name = 'Realtime allocations'; Exe = 'epinizer_realtime_tests.exe'; Args = '' },
  @{ Name = 'State and editor'; Exe = 'epinizer_plugin_tests.exe'; Args = '' },
  @{ Name = 'Actual VST3'; Exe = 'epinizer_vst3_host_tests.exe'; Args = '"' + (Join-Path $PSScriptRoot 'Epinizer.vst3') + '"' }
)
$failed = $false
foreach ($case in $cases) {
  $stdout = Join-Path $env:TEMP ('epinizer-' + [Guid]::NewGuid() + '.out')
  $stderr = $stdout + '.err'
  $options = @{ FilePath = (Join-Path $PSScriptRoot ('validation/' + $case.Exe)); PassThru = $true; RedirectStandardOutput = $stdout; RedirectStandardError = $stderr }
  if ($case.Args) { $options.ArgumentList = $case.Args }
  $process = Start-Process @options
  if (!$process.WaitForExit(60000)) {
    $process.Kill()
    $failed = $true
    ($case.Name + ': TIMEOUT') | Add-Content $resultFile
  } else {
    $process.WaitForExit()
    if ($process.ExitCode -ne 0) { $failed = $true }
    ($case.Name + ': exit ' + $process.ExitCode) | Add-Content $resultFile
  }
  Get-Content $stdout,$stderr -ErrorAction SilentlyContinue | Add-Content $resultFile
  Remove-Item $stdout,$stderr -ErrorAction SilentlyContinue
}
if ($failed) { 'VALIDATION FAILED' | Add-Content $resultFile } else { 'ALL TESTS PASSED' | Add-Content $resultFile }
Get-Content $resultFile
if ($failed) { exit 1 }
