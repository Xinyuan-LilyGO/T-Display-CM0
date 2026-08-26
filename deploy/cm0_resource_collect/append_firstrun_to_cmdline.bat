@echo off
setlocal

set "CMDLINE_PATH=%~dp0cmdline.txt"
set "FIRSTRUN_PATH=%~dp0firstrun.txt"

powershell -NoProfile -ExecutionPolicy Bypass -Command "$ErrorActionPreference='Stop'; $cmdPath=$env:CMDLINE_PATH; $firstPath=$env:FIRSTRUN_PATH; if (!(Test-Path -LiteralPath $cmdPath)) { throw 'cmdline.txt not found' }; if (!(Test-Path -LiteralPath $firstPath)) { throw 'firstrun.txt not found' }; $cmd = [IO.File]::ReadAllText($cmdPath).Trim() -replace '\s+', ' '; $first = [IO.File]::ReadAllText($firstPath).Trim() -replace '\s+', ' '; if ($first.Length -eq 0) { throw 'firstrun.txt is empty' }; if ($cmd -notlike ('*' + $first + '*')) { $cmd = ($cmd + ' ' + $first).Trim() }; [IO.File]::WriteAllText($cmdPath, $cmd, [Text.Encoding]::ASCII)"

if errorlevel 1 (
    echo Failed to update cmdline.txt
    pause
    exit /b 1
)

echo Updated cmdline.txt
