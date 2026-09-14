# Build-Abhängigkeiten

[English](DEPENDENCIES.md) | **Deutsch**

Portable Windows Tray Launcher verwendet C++17 und die Windows-API. Es gibt keine zusätzlich einzubindenden Anwendungsbibliotheken oder Paketdownloads im Build-Skript.

## Lokal vorhandener Werkzeugstand am 14. September 2026

| Werkzeug | Version / Variante |
| --- | --- |
| PowerShell | 7.6.5 |
| MinGW-w64 GCC / g++ | 16.2.0, Rev3, MSYS2, MINGW64/x86_64 |
| GNU windres / Binutils | 2.47.20260726 |
| C++-Sprachstandard | C++17 (`-std=c++17`) |

Installierte MSYS2-Paketversionen laut lokaler Paketdatenbank:

```text
mingw-w64-x86_64-gcc 16.2.0-3
mingw-w64-x86_64-binutils 2.47-3
mingw-w64-x86_64-headers 14.0.0.r262.g5ea8e9fac-1
mingw-w64-x86_64-crt 14.0.0.r262.g5ea8e9fac-1
```

Diese Angaben dokumentieren die bei der Sicherung vorhandenen Werkzeuge. Für die Sicherung wurde kein neuer Build und kein Test ausgeführt. Sie sind keine Zusage eines bitidentischen Builds mit anderen Werkzeugversionen.

## Benötigte Komponenten

- Windows 11, 64 Bit, als Zielsystem.
- Git zum Wiederherstellen des Repositorys.
- PowerShell 7 zum Ausführen der Skripte; `System.Drawing` unter Windows für die Icon-Konvertierung.
- MSYS2 mit den Paketen `mingw-w64-x86_64-gcc` und `mingw-w64-x86_64-binutils` einschließlich ihrer Paketabhängigkeiten (MinGW-w64-Header, CRT und Compilerbibliotheken).
- Standardpfad der Toolchain: `C:\msys64\mingw64\bin`. Ein anderer Pfad lässt sich mit `build.ps1 -Toolchain` angeben.

Gelinkt werden die mit Windows/MinGW bereitgestellten Systembibliotheken `shell32`, `shlwapi`, `ole32`, `uuid`, `advapi32`, `gdi32` und `user32`. Die GCC/C++-Laufzeit wird statisch eingebunden. Die fertige Anwendung benötigt keine separat mitzuliefernde Compiler-Runtime.

Quellen für die erzeugten Icons sind `AppIcon.png` und `TrayIcon-yellow.png`; beide werden mitgesichert. `Convert-Icon.ps1` erzeugt daraus die von `app.rc` referenzierten ICO-Dateien.
