# Build dependencies

**English** | [Deutsch](DEPENDENCIES.de.md)

Portable Windows Tray Launcher uses C++17 and the Windows API. The build script requires no additional application libraries and does not download packages.

## Locally installed tools recorded on September 14, 2026

| Tool | Version / variant |
| --- | --- |
| PowerShell | 7.6.5 |
| MinGW-w64 GCC / g++ | 16.2.0, Rev3, MSYS2, MINGW64/x86_64 |
| GNU windres / Binutils | 2.47.20260726 |
| C++ language standard | C++17 (`-std=c++17`) |

Installed MSYS2 package versions from the local package database:

```text
mingw-w64-x86_64-gcc 16.2.0-3
mingw-w64-x86_64-binutils 2.47-3
mingw-w64-x86_64-headers 14.0.0.r262.g5ea8e9fac-1
mingw-w64-x86_64-crt 14.0.0.r262.g5ea8e9fac-1
```

These versions document the tools present when the source was backed up. No new build or tests were run for that backup. This record does not guarantee a byte-identical build with other tool versions.

## Required components

- Windows 11, 64-bit, as the target operating system.
- Git to restore the repository.
- PowerShell 7 to run the scripts; `System.Drawing` on Windows for icon conversion.
- MSYS2 packages `mingw-w64-x86_64-gcc` and `mingw-w64-x86_64-binutils`, including their package dependencies (MinGW-w64 headers, CRT and compiler libraries).
- Default toolchain directory: `C:\msys64\mingw64\bin`. Use `build.ps1 -Toolchain` to specify another directory.

The application links against the system libraries supplied by Windows/MinGW: `shell32`, `shlwapi`, `ole32`, `uuid`, `advapi32`, `gdi32` and `user32`. The GCC/C++ runtime is linked statically, so the compiled application needs no separately distributed compiler runtime.

The generated icons come from `AppIcon.png` and `TrayIcon-yellow.png`, both included in the repository. `Convert-Icon.ps1` generates the ICO files referenced by `app.rc`.
