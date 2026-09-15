param([switch]$Shell, [switch]$Popup, [string]$Toolchain = 'C:\msys64\mingw64\bin')
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Split-Path $PSScriptRoot -Parent)
$env:PATH = "$Toolchain;$env:PATH"
$compiler = Join-Path $Toolchain 'g++.exe'
$release = Join-Path (Get-Location) 'build\PortableTrayLauncher.exe'
$fixture = Join-Path (Get-Location) 'build\Test Menü'
New-Item -ItemType Directory -Force -Path $fixture,"$fixture\01 Programme","$fixture\02 Dokumente\02 Unterordner","$fixture\03 Tools" | Out-Null
& $compiler '-std=c++17' '-Os' '-municode' '-mwindows' '-static' '-s' 'tests\probe.cpp' '-o' "$fixture\01 Programme\01 Testprogramm.exe" '-lshell32'
if ($LASTEXITCODE -ne 0) { throw 'Testprogramm-Build fehlgeschlagen' }
& $compiler '-std=c++17' '-Os' '-municode' '-static' '-Wall' '-Wextra' '-Wno-missing-field-initializers' 'tests\tests.cpp' 'build\app.o' '-o' 'build\tests.exe' '-lshell32' '-lshlwapi' '-lole32' '-luuid' '-ladvapi32' '-lgdi32' '-luser32'
if ($LASTEXITCODE -ne 0) { throw 'Test-Build fehlgeschlagen' }
$shellObject = New-Object -ComObject WScript.Shell
$shortcut = $shellObject.CreateShortcut("$fixture\01 Programme\02 Verknüpfung.lnk")
$shortcut.TargetPath = "$fixture\01 Programme\01 Testprogramm.exe"
$shortcut.Arguments = 'link.opened'
$shortcut.WorkingDirectory = "$fixture\01 Programme"
$shortcut.Save()
$broken = $shellObject.CreateShortcut("$fixture\01 Programme\04 Ungültig.lnk")
$broken.TargetPath = "$fixture\does-not-exist.exe"
$broken.Save()
Set-Content -LiteralPath "$fixture\01 Programme\03 Webseite.url" -Value "[InternetShortcut]`r`nURL=https://example.com/" -Encoding ascii
Set-Content -LiteralPath "$fixture\02 Dokumente\01 Handbuch.txt" -Value 'PortableTrayLauncher: Dokument erfolgreich über die Windows-Shell geöffnet.' -Encoding utf8
Set-Content -LiteralPath "$fixture\03 Tools\01 Stapel.bat" -Value '@echo off', '> "%~dp0bat.opened" echo OK' -Encoding ascii
Set-Content -LiteralPath "$fixture\03 Tools\02 Befehl.cmd" -Value '@echo off', '> "%~dp0cmd.opened" echo OK' -Encoding ascii
Set-Content -LiteralPath "$fixture\Zebra.txt" -Value 'Alphabetische Sortierung' -Encoding utf8
$metadata = "$fixture\desktop.ini"
if (!(Test-Path -LiteralPath $metadata)) { Set-Content -LiteralPath $metadata -Value '; hidden metadata' -Encoding ascii }
[System.IO.File]::SetAttributes($metadata, [System.IO.FileAttributes]::Hidden)
# Remove only known generated marker files, so repeated shell tests prove a fresh launch.
foreach ($marker in @('01 Programme\exe.opened','01 Programme\link.opened','03 Tools\bat.opened','03 Tools\cmd.opened')) {
  $path = Join-Path $fixture $marker
  if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
}
if ($Popup) {
  Start-Process -FilePath 'build\tests.exe' -ArgumentList "`"$fixture`" `"$release`" --popup" -WindowStyle Hidden
} elseif ($Shell) {
  & '.\build\tests.exe' $fixture $release '--shell'
  if ($LASTEXITCODE -ne 0) { throw 'Shell-Tests fehlgeschlagen' }
} else {
  & '.\build\tests.exe' $fixture $release
  if ($LASTEXITCODE -ne 0) { throw 'Tests fehlgeschlagen' }
}
