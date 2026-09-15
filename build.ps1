param([string]$Toolchain = 'C:\msys64\mingw64\bin')
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
& (Join-Path $PSScriptRoot 'Convert-Icon.ps1')
& (Join-Path $PSScriptRoot 'Convert-Icon.ps1') -Source (Join-Path $PSScriptRoot 'TrayIcon-yellow.png') -Destination (Join-Path $PSScriptRoot 'tray.ico')
$compiler = Join-Path $Toolchain 'g++.exe'
$windres = Join-Path $Toolchain 'windres.exe'
if (!(Test-Path -LiteralPath $compiler)) { throw 'MinGW-w64 fehlt. -Toolchain muss auf dessen bin-Ordner zeigen.' }
$env:PATH = "$Toolchain;$env:PATH"
New-Item -ItemType Directory -Force -Path 'build','dist' | Out-Null
& $windres '-i' 'app.rc' '-o' 'build\app.o'
if ($LASTEXITCODE -ne 0) { throw 'Ressourcen-Build fehlgeschlagen.' }
& $compiler '-std=c++17' '-Os' '-flto' '-Wall' '-Wextra' '-Wno-missing-field-initializers' '-municode' '-mwindows' '-static' '-static-libgcc' '-static-libstdc++' '-s' 'main.cpp' 'build\app.o' '-o' 'build\PortableTrayLauncher.exe' '-lshell32' '-lshlwapi' '-lole32' '-luuid' '-ladvapi32' '-lgdi32' '-luser32' '-Wl,--dynamicbase,--nxcompat'
if ($LASTEXITCODE -ne 0) { throw 'C++-Build fehlgeschlagen.' }
$version = (Get-Item -LiteralPath 'build\PortableTrayLauncher.exe').VersionInfo.FileVersion
if ($version -notmatch '^\d+\.\d+$') { throw 'Die Versionsnummer in app.rc muss das Format 1.2 haben.' }
$archiveName = "PortableTrayLauncher $version.zip"
$archivePath = Join-Path $PSScriptRoot "build\$archiveName"
$packageFiles = @('README.md','README.de.md','DEPENDENCIES.md','DEPENDENCIES.de.md','docs/images/launcher-menu.jpg','docs/images/documents-submenu.jpg')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archiveStream = [System.IO.File]::Open($archivePath, [System.IO.FileMode]::Create)
$archive = [System.IO.Compression.ZipArchive]::new($archiveStream, [System.IO.Compression.ZipArchiveMode]::Create)
try {
  $archive.CreateEntry('Menu/') | Out-Null
  [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, (Join-Path $PSScriptRoot 'build\PortableTrayLauncher.exe'), 'PortableTrayLauncher.exe') | Out-Null
  foreach ($file in $packageFiles) {
    [System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, (Join-Path $PSScriptRoot $file), $file) | Out-Null
  }
} finally { $archive.Dispose(); $archiveStream.Dispose() }

# Deliver one unpacked folder and one current ZIP; preserve the user's Menu.
$distPath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot 'dist'))
$applicationPath = Join-Path $distPath 'PortableTrayLauncher'
New-Item -ItemType Directory -Force -Path (Join-Path $applicationPath 'Menu'),(Join-Path $applicationPath 'docs\images') | Out-Null
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build\PortableTrayLauncher.exe') -Destination (Join-Path $applicationPath 'PortableTrayLauncher.exe') -Force
foreach ($file in $packageFiles) {
  Copy-Item -LiteralPath (Join-Path $PSScriptRoot $file) -Destination (Join-Path $applicationPath $file) -Force
}
$previousArchives = @(Get-ChildItem -LiteralPath $distPath -File | Where-Object { $_.Name -match '^PortableTrayLauncher \d+\.\d+\.zip$' })
if ($previousArchives.Count) {
  $backupPath = Join-Path $PSScriptRoot ('build\previous-packages\' + [guid]::NewGuid().ToString('N'))
  New-Item -ItemType Directory -Path $backupPath -Force | Out-Null
  foreach ($previous in $previousArchives) {
    if ($previous.DirectoryName -ne $distPath) { throw 'Unerwarteter Archivpfad.' }
    Move-Item -LiteralPath $previous.FullName -Destination $backupPath
  }
}
Copy-Item -LiteralPath $archivePath -Destination (Join-Path $distPath $archiveName)
Get-Item -LiteralPath $applicationPath,(Join-Path $distPath $archiveName) | Select-Object FullName,Length



