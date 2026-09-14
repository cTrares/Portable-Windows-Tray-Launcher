#define LAUNCHER_TEST
#include "../main.cpp"
#include <cstdio>
#include <stdexcept>

using namespace launcher;
int passed = 0;
void Check(bool result, const char* label) {
    if (!result) throw std::runtime_error(label);
    std::printf("PASS %s\n", label);
    ++passed;
}

struct RestoreStartup {
    DWORD type = 0, size = 0;
    std::vector<BYTE> bytes;
    bool existed = false;
    RestoreStartup() {
        HKEY key;
        LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, RunKey, 0, KEY_QUERY_VALUE, &key);
        if (status == ERROR_FILE_NOT_FOUND) return;
        if (status) throw std::runtime_error("Cannot back up startup entry");
        status = RegQueryValueExW(key, AppName, nullptr, &type, nullptr, &size);
        if (status == ERROR_SUCCESS) {
            bytes.resize(size);
            status = RegQueryValueExW(key, AppName, nullptr, &type, bytes.data(), &size);
            existed = status == ERROR_SUCCESS;
        }
        RegCloseKey(key);
        if (status != ERROR_SUCCESS && status != ERROR_FILE_NOT_FOUND)
            throw std::runtime_error("Cannot read startup entry");
    }
    ~RestoreStartup() {
        HKEY key;
        if (RegCreateKeyExW(HKEY_CURRENT_USER, RunKey, 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr)) return;
        if (existed) RegSetValueExW(key, AppName, 0, type, bytes.data(), size);
        else RegDeleteValueW(key, AppName);
        RegCloseKey(key);
    }
};

bool Exists(const std::wstring& path) { return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES; }
void WaitFile(const std::wstring& path, const char* label) {
    for (int i = 0; i < 100 && !Exists(path); ++i) Sleep(50);
    Check(Exists(path), label);
}

LRESULT CALLBACK TestWindow(HWND window, UINT message, WPARAM w, LPARAM l) {
    if (message == WM_LBUTTONUP || message == WM_RBUTTONUP) {
        POINT point;
        GetCursorPos(&point);
        ShowMenu(message == WM_RBUTTONUP, point);
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(window, &paint);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, Foreground);
        RECT bounds;
        GetClientRect(window, &bounds);
        DrawTextW(dc, L"Popup-Test: links = Launcher, rechts = Verwaltung", -1, &bounds,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        EndPaint(window, &paint);
        return 0;
    }
    return WindowProc(window, message, w, l);
}

int wmain(int count, wchar_t** args) {
    if (count < 3) { std::puts("tests.exe fixture-folder release-exe [--shell|--popup]"); return 2; }
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    menuPath = args[1];
    exePath = args[2];
    menuBrush = CreateSolidBrush(Background);
    try {
        if (count > 3 && wcscmp(args[3], L"--popup") == 0) {
            EnableDarkMenuFrame();
            WNDCLASSW klass{};
            klass.hInstance = GetModuleHandleW(nullptr);
            klass.lpfnWndProc = TestWindow;
            klass.hbrBackground = menuBrush;
            klass.lpszClassName = L"PortableTrayLauncher.TestWindow";
            RegisterClassW(&klass);
            owner = CreateWindowW(klass.lpszClassName, L"PortableTrayLauncher - Popup-Test",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 640, 420, nullptr, nullptr, klass.hInstance, nullptr);
            ShowWindow(owner, SW_SHOW);
            MSG message;
            while (GetMessageW(&message, nullptr, 0, 0) > 0) {
                TranslateMessage(&message); DispatchMessageW(&message);
            }
        } else {
            struct NameCase { const wchar_t* input; const wchar_t* expected; bool directory; };
            NameCase cases[] = {
                {L"01 Microsoft Word.lnk", L"Microsoft Word", false},
                {L"02-Excel.exe", L"Excel", false},
                {L"003_Handbuch.pdf", L"Handbuch", false},
                {L"4ChatGPT.url", L"ChatGPT", false},
                {L"05 Ordner.v2", L"Ordner.v2", true},
                {L"0 Favoriten", L"Favoriten", true},
                {L"1_Programme", L"Programme", true},
                {L"02-Dokumente", L"Dokumente", true},
                {L"3 Tools", L"Tools", true},
                {L"2026.txt", L"2026", false},
                {L"01_Änderung & Prüfung.txt", L"Änderung & Prüfung", false},
                {L"alpha.beta.txt", L"alpha.beta", false},
                {L".gitignore", L".gitignore", false}
            };
            for (const auto& test : cases) {
                Entry entry; entry.filename = test.input; entry.directory = test.directory; SetLabel(entry);
                Check(entry.label == test.expected, "clean display name");
            }
            DWORD error;
            auto entries = ReadDirectory(menuPath, error);
            Check(!error && entries.size() == 4, "fixture enumeration and hidden metadata filtering");
            Check(entries[0].label == L"Programme" && entries[1].label == L"Dokumente" &&
                entries[2].label == L"Tools" && entries[3].label == L"Zebra", "numeric then alphabetical order");
            std::vector<Entry> numbers;
            for (const wchar_t* name : {L"10 Z", L"2 Z", L"001 Z", L"9999999999999999999999999 Z"}) {
                Entry entry; entry.filename = name; entry.directory = true; SetLabel(entry); numbers.push_back(entry);
            }
            std::sort(numbers.begin(), numbers.end(), EntryLess);
            Check(numbers[0].order == L"1" && numbers[1].order == L"2" && numbers[2].order == L"10",
                "numeric prefixes sort without overflow");
            {
                std::vector<Entry> mixed;
                for (const auto& sample : {
                    NameCase{L"01 App.lnk", L"App", false},
                    NameCase{L"Folder", L"Folder", true},
                    NameCase{L"Alpha.txt", L"Alpha", false},
                    NameCase{L"10 Tools", L"Tools", true},
                    NameCase{L"2 Documents", L"Documents", true},
                    NameCase{L"Beta.txt", L"Beta", false}}) {
                    Entry entry;
                    entry.filename = sample.input;
                    entry.directory = sample.directory;
                    SetLabel(entry);
                    mixed.push_back(entry);
                }
                std::sort(mixed.begin(), mixed.end(), EntryLess);
                Check(mixed[0].label == L"Documents" && mixed[1].label == L"Tools" &&
                    mixed[2].label == L"Folder" && mixed[3].label == L"App" &&
                    mixed[4].label == L"Alpha" && mixed[5].label == L"Beta",
                    "folders precede numbered files; each group retains numeric and alphabetical order");
            }
            {
                Popup popup;
                popup.Populate(popup.root, menuPath, true);
                Check(GetMenuItemCount(popup.root) == 4, "root menu structure");
                HMENU programs = GetSubMenu(popup.root, 0);
                Check(programs && GetMenuItemCount(programs) == 4, "second-level submenu");
                HMENU docs = GetSubMenu(popup.root, 1);
                Check(docs && !GetSubMenu(docs, 1), "third-level folder has no nested menu");
                bool nestedFolder = false;
                for (const auto& item : popup.items) {
                    if (item->label == L"Unterordner") nestedFolder = !item->submenu && !item->path.empty();
                    if (!item->path.empty()) Check(item->icon != nullptr, "shell icon extracted");
                }
                Check(nestedFolder, "third-level folder opens as a path");
            }
            {
                Popup popup;
                popup.Populate(popup.root, menuPath + L"\\02 Dokumente\\02 Unterordner", true);
                Check(GetMenuItemCount(popup.root) == 2 && popup.items[0]->disabled &&
                    popup.items[1]->label == L"Menüordner öffnen", "empty menu with open-folder action");
            }
            // Every new population must see filesystem changes without a watcher.
            auto fresh = menuPath + L"\\04 Frisch.txt";
            HANDLE file = CreateFileW(fresh.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
            Check(file != INVALID_HANDLE_VALUE, "create refresh fixture");
            CloseHandle(file);
            Check(ReadDirectory(menuPath, error).size() == 5, "new entry visible on next read");
            DeleteFileW(fresh.c_str());
            DWORD before = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
            for (int i = 0; i < 100; ++i) { Popup popup; popup.Populate(popup.root, menuPath, true); }
            DWORD after = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
            Check(after <= before + 2, "100 menu openings do not leak GDI objects");
            {
                RestoreStartup restore;
                LSTATUS startupStatus = SetStartup(true);
                std::printf("Startup registry status: %ld\n", startupStatus);
                Check(startupStatus == ERROR_SUCCESS && StartupEnabled(), "current-user autostart enable and read-back");
                Check(SetStartup(false) == ERROR_SUCCESS && !StartupEnabled(), "current-user autostart disable");
            }
            if (count > 3 && wcscmp(args[3], L"--shell") == 0) {
                std::wstring programs = menuPath + L"\\01 Programme\\";
                Check(OpenPath(programs + L"01 Testprogramm.exe", false), "EXE shell launch accepted");
                WaitFile(programs + L"exe.opened", "EXE actually ran");
                Check(OpenPath(programs + L"02 Verknüpfung.lnk", false), "LNK shell launch accepted");
                WaitFile(programs + L"link.opened", "LNK target and arguments executed");
                Check(OpenPath(menuPath + L"\\03 Tools\\01 Stapel.bat", false), "BAT shell launch accepted");
                WaitFile(menuPath + L"\\03 Tools\\bat.opened", "BAT actually ran");
                Check(OpenPath(menuPath + L"\\03 Tools\\02 Befehl.cmd", false), "CMD shell launch accepted");
                WaitFile(menuPath + L"\\03 Tools\\cmd.opened", "CMD actually ran");
                Check(OpenPath(programs + L"03 Webseite.url", false), "URL default browser launch accepted");
                Check(OpenPath(menuPath + L"\\02 Dokumente\\01 Handbuch.txt", false), "document default app launch accepted");
                Check(OpenPath(menuPath + L"\\02 Dokumente\\02 Unterordner", false), "folder Explorer launch accepted");
                Check(!OpenPath(programs + L"04 Ungültig.lnk", false), "broken shortcut rejected without crash");
            }
            std::printf("%d checks passed. Startup entry restored.\n", passed);
        }
    } catch (const std::exception& exception) {
        std::fprintf(stderr, "FAIL %s (Windows error %lu)\n", exception.what(), GetLastError());
        return 1;
    }
    if (menuFont) DeleteObject(menuFont);
    DeleteObject(menuBrush);
    CoUninitialize();
    return 0;
}
