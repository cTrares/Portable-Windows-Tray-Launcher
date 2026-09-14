param([string]$Toolchain = 'C:\msys64\mingw64\bin')
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
& (Join-Path $PSScriptRoot 'Convert-Icon.ps1')
& (Join-Path $PSScriptRoot 'Convert-Icon.ps1') -Source (Join-Path $PSScriptRoot 'TrayIcon-yellow.png') -Destination (Join-Path $PSScriptRoot 'tray.ico')
$compiler = Join-Path $Toolchain 'g++.exe'
$windres = Join-Path $Toolchain 'windres.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw 'MinGW-w64 fehlt. -Toolchain muss auf dessen bin-Ordner zeigen.' }
$env:PATH = "$Toolchain;$env:PATH"
New-Item -ItemType Directory -Force -Path 'build','dist\PortableTrayLauncher\Menu' | Out-Null
& $windres '-i' 'app.rc' '-o' 'build\app.o'
if ($LASTEXITCODE -ne 0) { throw 'Ressourcen-Build fehlgeschlagen.' }
& $compiler '-std=c++17' '-Os' '-flto' '-Wall' '-Wextra' '-Wno-missing-field-initializers' '-municode' '-mwindows' '-static' '-static-libgcc' '-static-libstdc++' '-s' 'main.cpp' 'build\app.o' '-o' 'dist\PortableTrayLauncher\PortableTrayLauncher.exe' '-lshell32' '-lshlwapi' '-lole32' '-luuid' '-ladvapi32' '-lgdi32' '-luser32' '-Wl,--dynamicbase,--nxcompat'
if ($LASTEXITCODE -ne 0) { throw 'C++-Build fehlgeschlagen.' }
if (Test-Path -LiteralPath 'README.md') { Copy-Item -LiteralPath 'README.md' -Destination 'dist\PortableTrayLauncher\README.md' -Force }
Get-Item -LiteralPath 'dist\PortableTrayLauncher\PortableTrayLauncher.exe' | Select-Object FullName,Length



