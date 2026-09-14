# Portable Windows Tray Launcher

[English](README.md) | **Deutsch**

Ein portabler Programmstarter für Windows 11: Programme, Verknüpfungen, Dateien und Ordner direkt aus dem Infobereich der Taskleiste öffnen. Einfach den Menüordner befüllen – daraus entsteht automatisch ein übersichtliches Startmenü mit Untermenüs, Dateisymbolen und eigener Sortierung. Optional mit Autostart, ohne Installation und ohne zusätzliche Runtime.

**Portable Windows system tray launcher** for apps, shortcuts, files and folders. A folder-based quick launch menu with submenus, dark styling and optional startup with Windows. Native C++/Win32, 64-bit; no installer or extra runtime required.

## Download

**[Portable-ZIP für Windows 64 Bit herunterladen](https://github.com/cTrares/Portable-Windows-Tray-Launcher/releases/latest/download/Portable-Windows-Tray-Launcher-win64.zip)**

Die gesamte ZIP in einen beschreibbaren Ordner entpacken und darin `PortableTrayLauncher.exe` starten. Kein Build und keine Installation erforderlich. [Versionshinweise und Downloads](https://github.com/cTrares/Portable-Windows-Tray-Launcher/releases/latest).

## Benutzung

`dist\PortableTrayLauncher\PortableTrayLauncher.exe` starten. Das Programm läuft ausschließlich im Infobereich der Taskleiste. Windows kann neue Tray-Icons zunächst unter dem Aufklapppfeil verbergen; das Icon lässt sich von dort in den sichtbaren Infobereich ziehen.

- Linksklick: Menü unmittelbar oberhalb des Tray-Icons öffnen.
- Rechtsklick: Menüordner öffnen, Autostart aktivieren/deaktivieren, Beenden.
- Konfiguration: Dateien, Verknüpfungen und Ordner in `Menu` neben der EXE ablegen. Beim nächsten Linksklick wird alles frisch eingelesen.
- Kein Hauptfenster, kein Einstellungsfenster, kein Menüeditor, keine permanente Ordnerüberwachung.

Der gesamte Release-Ordner kann kopiert oder verschoben werden. Verknüpfungen und deren Ziele müssen am neuen Ort weiterhin gültig sein. Bei aktiviertem Autostart nach einem Umzug am neuen Ort erneut „Autostart aktivieren“ wählen: Windows benötigt dort einen absoluten EXE-Pfad. Autostart wird pro Benutzer unter `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` gespeichert; Administratorrechte sind nicht nötig. Es wird nicht automatisch Autostart aktiviert.

## Menüstruktur

Die erste Ordnerstufe wird als Untermenü angezeigt. Darin enthaltene weitere Ordner öffnen sich im Explorer. Dateien dürfen auch direkt in `Menu` liegen. Versteckte/System-Dateien wie `desktop.ini` werden ausgelassen.

Dateiendungen werden ausgeblendet. Führende Ziffern mit optionalen Leerzeichen, Bindestrichen oder Unterstrichen dienen als Sortiernummer, z. B. `01 Programme`, `02-Excel.lnk`, `03_Handbuch.pdf`. Nummerierte Einträge erscheinen zuerst, numerisch sortiert; anschließend folgen unnummerierte Einträge alphabetisch. Reine Zahlennamen bleiben lesbar.

Alle Einträge starten über die registrierte Standardaktion der Windows-Shell. Deshalb wird beispielsweise eine PS1-Datei nur ausgeführt, wenn Windows dies auch beim Doppelklick tun würde. Verknüpfungen behalten Argumente und Arbeitsverzeichnis. Fehlgeschlagene Starts zeigen eine kleine native Fehlermeldung.

Ein leerer Ordner bietet „Menü ist leer“ und „Menüordner öffnen“. Tastatur: Pfeiltasten, Enter, Escape und Anfangsbuchstaben. Große Menüs verwenden die native Windows-Menünavigation.

## Gestaltung

Neutrales Windows-Dark-Grau: Hintergrund `#202020`, Hervorhebung `#2D2D2D`, Text `#F3F3F3`. Kompakte 30-DIP-Zeilen, Segoe UI, Shell-Dateiicons, DPI-Skalierung. Das bereitgestellte `AppIcon.png` ist als mehrstufiges ICO in der EXE eingebettet. Das Original bleibt unverändert.

Das Menü ist ein echtes Win32-Popup und wird an der tatsächlichen Position des Tray-Icons verankert. Die Einblendanimation richtet sich nach den Windows-Einstellungen; das Programm fordert die Öffnungsrichtung von unten nach oben an.

## Wiederherstellung und Build

Das Repository enthält den vollständigen Quellstand einschließlich Build-Skripten, Windows-Ressourcen und PNG-Iconquellen. Compiler, installierte Bibliotheken und fertige EXE-Dateien werden nicht in Git aufgenommen; fertige Portable-ZIPs stehen als Release-Downloads bereit. Die Versionen der lokalen Build-Werkzeuge stehen in [DEPENDENCIES.de.md](DEPENDENCIES.de.md).

Auf einem neuen Windows-Rechner Git, PowerShell 7 und MSYS2 mit der MinGW-w64-Toolchain für **MINGW64/x86_64** bereitstellen. In einer MSYS2-MINGW64-Shell die Build-Abhängigkeiten installieren:

```sh
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-binutils
```

Danach das Repository klonen, in dessen Ordner wechseln und in PowerShell 7 ausführen:

```powershell
git clone https://github.com/cTrares/Portable-Windows-Tray-Launcher.git
cd Portable-Windows-Tray-Launcher
.\build.ps1
# Anderer Compilerpfad:
.\build.ps1 -Toolchain 'C:\msys64\mingw64\bin'
```

Das Build-Skript erzeugt `app.ico` und `tray.ico` aus den versionierten PNG-Dateien sowie alle Build- und Ausgabeordner. Die C++-Runtime wird statisch eingebunden. Ausgabe: `dist\PortableTrayLauncher`. Der technische EXE-Name lautet `PortableTrayLauncher.exe`. Für den Betrieb werden nur EXE und `Menu` benötigt.

Der persönliche `Menu`-Inhalt gehört nicht zum Quellstand. Er wird beim Build leer angelegt und kann anschließend mit eigenen Verknüpfungen und Dateien befüllt werden. Für die Wiederherstellung einer persönlichen Einrichtung muss dieser Inhalt separat gesichert werden; installierte oder portable Zielprogramme werden separat bereitgestellt.

## Tests

```powershell
.\tests\run.ps1
.\tests\run.ps1 -Shell
```

Die Shell-Tests öffnen ausdrücklich harmlose Testdateien, einen Browser und Explorer. Ein vorbestehender eigener Autostart-Eintrag wird gesichert und nach dem Test wiederhergestellt. Die Registry-Tests benötigen einen normalen Benutzerprozess außerhalb einer schreibgeschützten Testsandbox, keine Administratorrechte.

Die Testquellen sind im Repository enthalten; Testprogramme und Testdaten werden bei Bedarf erzeugt. `tests` und `build` werden für den normalen Betrieb nicht benötigt.
