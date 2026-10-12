# Run directly, or double-click Install-Epinizer.cmd. UAC elevation is requested.
$ErrorActionPreference = 'Stop'
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($identity)
if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
  $arguments = '-NoProfile -ExecutionPolicy Bypass -File "' + $PSCommandPath + '"'
  $process = Start-Process powershell.exe -Verb RunAs -ArgumentList $arguments -Wait -PassThru
  exit $process.ExitCode
}
$source = Join-Path $PSScriptRoot 'Epinizer.vst3'
if (!(Test-Path $source)) { throw 'Epinizer.vst3 missing. Extract the complete distribution first.' }
$destination = Join-Path $env:CommonProgramFiles 'VST3'
New-Item -ItemType Directory -Force $destination | Out-Null
$target = Join-Path $destination 'Epinizer.vst3'
if (Test-Path $target) {
  $backup = Join-Path $env:LOCALAPPDATA ('Epinizer/Backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
  New-Item -ItemType Directory -Force $backup | Out-Null
  Move-Item $target $backup
}
Copy-Item $source $destination -Recurse
Write-Host 'Epinizer VST3 installed. Restart your DAW and rescan plug-ins.'
