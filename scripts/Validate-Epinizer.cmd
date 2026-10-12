@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0validate-windows.ps1"
set result=%errorlevel%
pause
exit /b %result%
