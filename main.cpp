#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// No configuration files: the Menu directory beside this executable is the editor.
namespace launcher {
constexpr wchar_t AppName[] = L"PortableTrayLauncher";
constexpr wchar_t WindowClass[] = L"PortableTrayLauncher.MessageWindow";
constexpr wchar_t RunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr UINT TrayMessage = WM_APP + 1;
constexpr COLORREF Background = RGB(32, 32, 32);
constexpr COLORREF Hover = RGB(45, 45, 45);
constexpr COLORREF Foreground = RGB(243, 243, 243);
constexpr UINT OpenMenu = 1, ToggleStartup = 2, Quit = 3, FirstEntry = 100;

HWND owner = nullptr;
std::wstring exePath, menuPath;
HICON trayIcon = nullptr;
HFONT menuFont = nullptr;
HBRUSH menuBrush = nullptr;
UINT dpi = 96, taskbarCreated = 0;
bool menuOpen = false, trayAdded = false;

std::wstring Parent(const std::wstring& path) {
    auto slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? L"" : path.substr(0, slash + 1);
}

std::wstring ExecutablePath() {
    std::wstring path(32768, L'\0');
    DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (!length || length >= path.size()) return {};
    path.resize(length);
    return path;
}

void Error(const std::wstring& text, DWORD code) {
    wchar_t* message = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, code, 0,
        reinterpret_cast<wchar_t*>(&message), 0, nullptr);
    std::wstring detail = text + L"\n\n" + (message ? message : L"Windows konnte den Vorgang nicht ausführen.");
    if (message) LocalFree(message);
    MessageBoxW(owner, detail.c_str(), AppName, MB_OK | MB_ICONERROR);
}

bool EnsureMenu(bool report = true) {
    if (CreateDirectoryW(menuPath.c_str(), nullptr)) return true;
    DWORD error = GetLastError();
    DWORD attrs = GetFileAttributesW(menuPath.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) return true;
    if (report) Error(L"Der Menüordner konnte nicht angelegt werden:\n" + menuPath, error);
    return false;
}

// NULL verb delegates to the same registered default action used by Explorer.
// A PS1, for example, may open in an editor if that is the user's association.
bool OpenPath(const std::wstring& path, bool report = true) {
    SHELLEXECUTEINFOW info{sizeof(info)};
    info.fMask = SEE_MASK_NOASYNC | SEE_MASK_FLAG_NO_UI;
    info.hwnd = owner;
    info.lpFile = path.c_str();
    std::wstring workingDirectory = Parent(path);
    info.lpDirectory = workingDirectory.c_str();
    info.nShow = SW_SHOWNORMAL;
    if (ShellExecuteExW(&info)) return true;
    DWORD error = GetLastError();
    if (report) Error(L"Der Eintrag konnte nicht geöffnet werden:\n" + path, error);
    return false;
}

std::wstring StartupCommand() { return L"\"" + exePath + L"\""; }

bool StartupEnabled() {
    wchar_t value[32768]{};
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, RunKey, AppName, RRF_RT_REG_SZ,
        nullptr, value, &size) != ERROR_SUCCESS) return false;
    return _wcsicmp(value, StartupCommand().c_str()) == 0;
}

LSTATUS SetStartup(bool enabled) {
    HKEY key = nullptr;
    LSTATUS status = RegCreateKeyExW(HKEY_CURRENT_USER, RunKey, 0, nullptr, 0,
        KEY_SET_VALUE, nullptr, &key, nullptr);
    if (status != ERROR_SUCCESS) return status;
    if (enabled) {
        std::wstring command = StartupCommand();
        status = RegSetValueExW(key, AppName, 0, REG_SZ,
            reinterpret_cast<const BYTE*>(command.c_str()),
            static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        status = RegDeleteValueW(key, AppName);
        if (status == ERROR_FILE_NOT_FOUND) status = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    return status;
}

struct Entry {
    std::wstring filename, path, label, order;
    bool directory = false;
};

// Keep numbers as strings to sort even arbitrarily long prefixes without overflow.
void SetLabel(Entry& entry) {
    std::wstring stem = entry.filename;
    if (!entry.directory) {
        auto dot = stem.find_last_of(L'.');
        if (dot != std::wstring::npos && dot != 0) stem.resize(dot);
    }
    size_t digits = 0;
    while (digits < stem.size() && stem[digits] >= L'0' && stem[digits] <= L'9') ++digits;
    size_t start = digits;
    while (start < stem.size() && (stem[start] == L' ' || stem[start] == L'-' || stem[start] == L'_')) ++start;
    // Do not turn a name made only of digits/separators into a blank row.
    if (digits && start < stem.size()) {
        entry.order = stem.substr(0, digits);
        auto nonzero = entry.order.find_first_not_of(L'0');
        entry.order = nonzero == std::wstring::npos ? L"0" : entry.order.substr(nonzero);
        entry.label = stem.substr(start);
    } else {
        entry.label = stem;
    }
}

bool EntryLess(const Entry& a, const Entry& b) {
    if (a.directory != b.directory) return a.directory;
    if (a.order.empty() != b.order.empty()) return !a.order.empty();
    if (a.order != b.order) {
        if (a.order.size() != b.order.size()) return a.order.size() < b.order.size();
        return a.order < b.order;
    }
    int comparison = CompareStringEx(LOCALE_NAME_USER_DEFAULT, NORM_IGNORECASE,
        a.label.c_str(), -1, b.label.c_str(), -1, nullptr, nullptr, 0);
    if (comparison == CSTR_LESS_THAN) return true;
    if (comparison == CSTR_GREATER_THAN) return false;
    return a.filename < b.filename;
}

std::vector<Entry> ReadDirectory(const std::wstring& path, DWORD& error) {
    std::vector<Entry> entries;
    error = ERROR_SUCCESS;
    WIN32_FIND_DATAW data{};
    HANDLE search = FindFirstFileW((path + L"\\*").c_str(), &data);
    if (search == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        if (error == ERROR_FILE_NOT_FOUND) error = ERROR_SUCCESS;
        return entries;
    }
    do {
        if (wcscmp(data.cFileName, L".") == 0 || wcscmp(data.cFileName, L"..") == 0) continue;
        // Explorer metadata such as desktop.ini is not a launcher entry.
        if (data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) continue;
        Entry entry;
        entry.filename = data.cFileName;
        entry.path = path + L"\\" + data.cFileName;
        entry.directory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        SetLabel(entry);
        entries.push_back(std::move(entry));
    } while (FindNextFileW(search, &data));
    DWORD lastError = GetLastError();
    FindClose(search);
    if (lastError != ERROR_NO_MORE_FILES) error = lastError;
    std::sort(entries.begin(), entries.end(), EntryLess);
    return entries;
}

int Scale(int value) { return MulDiv(value, static_cast<int>(dpi), 96); }

HICON ShellIcon(const std::wstring& path) {
    SHFILEINFOW info{};
    // Shell owns extraction/association/shortcut resolution; we own the returned icon.
    if (SHGetFileInfoW(path.c_str(), 0, &info, sizeof(info), SHGFI_ICON | SHGFI_SMALLICON))
        return info.hIcon;
    return nullptr;
}

struct MenuItem {
    UINT id = 0;
    std::wstring label, path;
    HICON icon = nullptr;
    bool submenu = false, disabled = false, checked = false;
    ~MenuItem() { if (icon) DestroyIcon(icon); }
};

// Item pointers must survive the native modal menu loop and every submenu.
struct Popup {
    HMENU root = nullptr;
    std::vector<std::unique_ptr<MenuItem>> items;
    UINT nextId = FirstEntry;
    Popup() { root = NewMenu(); }
    ~Popup() { if (root) DestroyMenu(root); }
    static HMENU NewMenu() {
        HMENU menu = CreatePopupMenu();
        MENUINFO info{sizeof(info)};
        info.fMask = MIM_BACKGROUND | MIM_STYLE;
        info.hbrBack = menuBrush;
        info.dwStyle = MNS_NOCHECK;
        SetMenuInfo(menu, &info);
        return menu;
    }
    void Add(HMENU menu, const std::wstring& label, UINT id = 0,
        const std::wstring& path = {}, HMENU submenu = nullptr,
        bool disabled = false, bool checked = false) {
        auto item = std::make_unique<MenuItem>();
        item->id = id;
        item->label = label;
        item->path = path;
        item->submenu = submenu != nullptr;
        item->disabled = disabled;
        item->checked = checked;
        if (!path.empty()) item->icon = ShellIcon(path);
        MENUITEMINFOW info{sizeof(info)};
        info.fMask = MIIM_FTYPE | MIIM_STATE | MIIM_ID | MIIM_DATA | MIIM_STRING;
        info.fType = MFT_OWNERDRAW;
        info.fState = disabled ? MFS_DISABLED : MFS_ENABLED;
        info.wID = id;
        info.dwItemData = reinterpret_cast<ULONG_PTR>(item.get());
        info.dwTypeData = item->label.data(); // Keep a native accessible text label, too.
        if (submenu) { info.fMask |= MIIM_SUBMENU; info.hSubMenu = submenu; }
        InsertMenuItemW(menu, GetMenuItemCount(menu), TRUE, &info);
        items.push_back(std::move(item));
    }
    void Populate(HMENU menu, const std::wstring& path, bool topLevel) {
        DWORD error;
        auto entries = ReadDirectory(path, error);
        if (error) Add(menu, L"Ordner nicht lesbar", 0, {}, nullptr, true);
        if (entries.empty()) {
            if (!error) Add(menu, L"Menü ist leer", 0, {}, nullptr, true);
            Add(menu, L"Menüordner öffnen", nextId++, path);
        }
        for (const auto& entry : entries) {
            HMENU child = nullptr;
            if (topLevel && entry.directory) {
                child = NewMenu();
                Populate(child, entry.path, false);
            }
            // Below the second visible level, directories open in Explorer.
            Add(menu, entry.label, nextId++, entry.path, child);
        }
    }
};

void SetMenuFont(POINT point) {
    if (!(GetWindowLongW(owner, GWL_STYLE) & WS_CAPTION)) SetWindowPos(owner, nullptr, point.x, point.y, 1, 1,
        SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOREDRAW);
    dpi = GetDpiForWindow(owner);
    if (!dpi) dpi = 96;
    if (menuFont) DeleteObject(menuFont);
    menuFont = CreateFontW(-Scale(13), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
}

void Measure(MEASUREITEMSTRUCT* measure) {
    auto* item = reinterpret_cast<MenuItem*>(measure->itemData);
    if (!item) return;
    HDC dc = GetDC(owner);
    HGDIOBJ old = SelectObject(dc, menuFont);
    SIZE size{};
    GetTextExtentPoint32W(dc, item->label.c_str(), static_cast<int>(item->label.size()), &size);
    SelectObject(dc, old);
    ReleaseDC(owner, dc);
    measure->itemWidth = std::clamp(static_cast<int>(size.cx) + Scale(56), Scale(170), Scale(420));
    measure->itemHeight = Scale(30);
}

void Draw(DRAWITEMSTRUCT* draw) {
    auto* item = reinterpret_cast<MenuItem*>(draw->itemData);
    if (!item) return;
    HDC dc = draw->hDC;
    int saved = SaveDC(dc);
    RECT row = draw->rcItem;
    FillRect(dc, &row, menuBrush);
    if ((draw->itemState & ODS_SELECTED) && !item->disabled) {
        RECT inset = row;
        InflateRect(&inset, -Scale(3), -Scale(2));
        HBRUSH brush = CreateSolidBrush(Hover);
        SelectObject(dc, brush);
        SelectObject(dc, GetStockObject(NULL_PEN));
        RoundRect(dc, inset.left, inset.top, inset.right, inset.bottom, Scale(5), Scale(5));
        SelectObject(dc, GetStockObject(NULL_BRUSH));
        DeleteObject(brush);
    }
    int iconSize = Scale(16);
    int centerY = (row.top + row.bottom) / 2;
    if (item->icon) DrawIconEx(dc, row.left + Scale(10), centerY - iconSize / 2,
        item->icon, iconSize, iconSize, 0, nullptr, DI_NORMAL);
    if (item->checked) {
        HPEN pen = CreatePen(PS_SOLID, Scale(2), Foreground);
        HGDIOBJ old = SelectObject(dc, pen);
        MoveToEx(dc, row.left + Scale(12), centerY, nullptr);
        LineTo(dc, row.left + Scale(16), centerY + Scale(4));
        LineTo(dc, row.left + Scale(23), centerY - Scale(4));
        SelectObject(dc, old);
        DeleteObject(pen);
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, item->disabled ? RGB(155, 155, 155) : Foreground);
    SelectObject(dc, menuFont);
    RECT text = row;
    text.left += Scale(36);
    text.right -= Scale(25);
    DrawTextW(dc, item->label.c_str(), -1, &text,
        DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    if (item->submenu) {
        HPEN pen = CreatePen(PS_SOLID, std::max(1, Scale(1)), Foreground);
        HGDIOBJ old = SelectObject(dc, pen);
        int x = row.right - Scale(13);
        MoveToEx(dc, x - Scale(2), centerY - Scale(3), nullptr);
        LineTo(dc, x + Scale(1), centerY);
        LineTo(dc, x - Scale(2), centerY + Scale(3));
        SelectObject(dc, old);
        DeleteObject(pen);
    }
    RestoreDC(dc, saved);
    // USER32 adds its triangle after WM_DRAWITEM, even for owner-drawn items.
    // Keep this row out of that final draw so only our light chevron remains.
    if (item->submenu) ExcludeClipRect(dc, row.left, row.top, row.right, row.bottom);
}

void ShowMenu(bool administration, POINT point) {
    if (menuOpen) return;
    menuOpen = true;
    SetMenuFont(point);
    Popup popup;
    if (administration) {
        popup.Add(popup.root, L"Menüordner öffnen", OpenMenu, menuPath);
        bool enabled = StartupEnabled();
        popup.Add(popup.root, enabled ? L"Autostart deaktivieren" : L"Autostart aktivieren",
            ToggleStartup, {}, nullptr, false, enabled);
        popup.Add(popup.root, L"Beenden", Quit);
    } else if (EnsureMenu()) {
        popup.Populate(popup.root, menuPath, true);
    } else {
        menuOpen = false;
        return;
    }
    // Required for tray menus to dismiss correctly on a click elsewhere.
    SetForegroundWindow(owner);
    UINT command = TrackPopupMenuEx(popup.root,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_VERNEGANIMATION,
        point.x, point.y, owner, nullptr);
    PostMessageW(owner, WM_NULL, 0, 0);
    menuOpen = false;
    if (command == Quit) DestroyWindow(owner);
    else if (command == OpenMenu) { if (EnsureMenu()) OpenPath(menuPath); }
    else if (command == ToggleStartup) {
        LSTATUS status = SetStartup(!StartupEnabled());
        if (status != ERROR_SUCCESS) Error(L"Autostart konnte nicht geändert werden.", status);
    } else if (command >= FirstEntry) {
        for (const auto& item : popup.items) {
            if (item->id == command) { OpenPath(item->path); break; }
        }
    }
}

// Windows 11's dark menu frame is not exposed by a public Win32 switch.
// Optional process-local theme opt-in; owner-drawn rows also work if unavailable.
void EnableDarkMenuFrame() {
    HMODULE theme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!theme) return;
    using SetMode = int (WINAPI*)(int);
    using FlushThemes = void (WINAPI*)();
    auto setMode = reinterpret_cast<SetMode>(GetProcAddress(theme, MAKEINTRESOURCEA(135)));
    auto flush = reinterpret_cast<FlushThemes>(GetProcAddress(theme, MAKEINTRESOURCEA(136)));
    if (setMode && flush) { setMode(2); flush(); } // ForceDark, Windows 10 1903 / Windows 11.
    // Keep uxtheme loaded until process exit: its process-local state backs the menus.
}
HICON LoadTrayIcon() {
    HWND taskbar = FindWindowW(L"Shell_TrayWnd", nullptr);
    UINT taskbarDpi = taskbar ? GetDpiForWindow(taskbar) : GetDpiForSystem();
    if (!taskbarDpi) taskbarDpi = 96;
    int size = GetSystemMetricsForDpi(SM_CXSMICON, taskbarDpi);
    return static_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(2),
        IMAGE_ICON, size, size, LR_DEFAULTCOLOR));
}

void RefreshTrayIcon() {
    HICON replacement = LoadTrayIcon();
    if (!replacement) return;
    NOTIFYICONDATAW data{sizeof(data)};
    data.hWnd = owner;
    data.uID = 1;
    data.uFlags = NIF_ICON;
    data.hIcon = replacement;
    if (Shell_NotifyIconW(NIM_MODIFY, &data)) {
        if (trayIcon) DestroyIcon(trayIcon);
        trayIcon = replacement;
    } else DestroyIcon(replacement);
}
bool AddTrayIcon() {
    HICON replacement = LoadTrayIcon();
    if (replacement) { if (trayIcon) DestroyIcon(trayIcon); trayIcon = replacement; }
    NOTIFYICONDATAW data{sizeof(data)};
    data.hWnd = owner;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data.uCallbackMessage = TrayMessage;
    data.hIcon = trayIcon;
    wcscpy_s(data.szTip, AppName);
    if (!Shell_NotifyIconW(NIM_ADD, &data)) return false;
    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
    trayAdded = true;
    return true;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (taskbarCreated && message == taskbarCreated) {
        trayAdded = false;
        if (!AddTrayIcon()) SetTimer(window, 1, 2000, nullptr);
        return 0;
    }
    switch (message) {
    case WM_SETTINGCHANGE:
    case WM_DISPLAYCHANGE:
        if (trayAdded) RefreshTrayIcon();
        return 0;
    case WM_TIMER:
        if (wParam == 1 && AddTrayIcon()) KillTimer(window, 1);
        return 0;
    case TrayMessage: {
        UINT event = LOWORD(lParam);
        if (event == NIN_SELECT || event == NIN_KEYSELECT || event == WM_CONTEXTMENU) {
            POINT point{GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)};
            if (point.x == -1 && point.y == -1) GetCursorPos(&point);
            NOTIFYICONIDENTIFIER tray{sizeof(tray)};
            tray.hWnd = owner;
            tray.uID = 1;
            RECT iconRect{};
            if (SUCCEEDED(Shell_NotifyIconGetRect(&tray, &iconRect))) {
                point.x = iconRect.right;
                point.y = iconRect.top - MulDiv(4, GetDpiForWindow(owner), 96);
            }
            ShowMenu(event == WM_CONTEXTMENU, point);
        }
        return 0;
    }
    case WM_MEASUREITEM:
        if (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlType == ODT_MENU) {
            Measure(reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)); return TRUE;
        }
        break;
    case WM_DRAWITEM:
        if (reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->CtlType == ODT_MENU) {
            Draw(reinterpret_cast<DRAWITEMSTRUCT*>(lParam)); return TRUE;
        }
        break;
    case WM_MENUCHAR: {
        // Native arrow/Enter/Esc navigation is automatic. Add first-letter navigation
        // because Windows cannot infer it for owner-drawn items.
        HMENU menu = reinterpret_cast<HMENU>(lParam);
        int count = GetMenuItemCount(menu), selected = -1;
        for (int i = 0; i < count; ++i)
            if (GetMenuState(menu, i, MF_BYPOSITION) & MF_HILITE) selected = i;
        for (int offset = 1; offset <= count; ++offset) {
            int i = (selected + offset) % count;
            MENUITEMINFOW info{sizeof(info)};
            info.fMask = MIIM_DATA;
            GetMenuItemInfoW(menu, i, TRUE, &info);
            auto* item = reinterpret_cast<MenuItem*>(info.dwItemData);
            wchar_t key = static_cast<wchar_t>(LOWORD(wParam));
            if (item && !item->disabled && !item->label.empty() &&
                CompareStringOrdinal(item->label.c_str(), 1, &key, 1, TRUE) == CSTR_EQUAL)
                return MAKELRESULT(i, MNC_SELECT);
        }
        return MAKELRESULT(0, MNC_IGNORE);
    }
    case WM_DESTROY: {
        NOTIFYICONDATAW data{sizeof(data)};
        data.hWnd = window;
        data.uID = 1;
        Shell_NotifyIconW(NIM_DELETE, &data);
        KillTimer(window, 1);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace launcher

#ifndef LAUNCHER_TEST
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    using namespace launcher;
    exePath = ExecutablePath();
    if (exePath.empty()) return 1;
    menuPath = Parent(exePath) + L"Menu";
    // One process per portable folder, without a persistent lock file.
    std::wstring normalizedPath = exePath;
    CharLowerBuffW(normalizedPath.data(), static_cast<DWORD>(normalizedPath.size()));
    unsigned long long hash = 14695981039346656037ULL;
    for (wchar_t c : normalizedPath) { hash ^= c; hash *= 1099511628211ULL; }
    std::wstring mutexName = L"Local\\PortableTrayLauncher:" + std::to_wstring(hash);
    HANDLE mutex = CreateMutexW(nullptr, FALSE, mutexName.c_str());
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mutex); return 0; }
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (!EnsureMenu()) { if (SUCCEEDED(com)) CoUninitialize(); if (mutex) CloseHandle(mutex); return 1; }
    EnableDarkMenuFrame();
    trayIcon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(1), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    menuBrush = CreateSolidBrush(Background);
    WNDCLASSW klass{};
    klass.lpfnWndProc = WindowProc;
    klass.hInstance = instance;
    klass.lpszClassName = WindowClass;
    klass.hIcon = trayIcon;
    RegisterClassW(&klass);
    owner = CreateWindowExW(WS_EX_TOOLWINDOW, WindowClass, AppName, WS_POPUP,
        0, 0, 1, 1, nullptr, nullptr, instance, nullptr);
    taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    int result = 1;
    if (owner) {
        if (!AddTrayIcon()) SetTimer(owner, 1, 2000, nullptr);
        MSG message{};
        int status;
        while ((status = GetMessageW(&message, nullptr, 0, 0)) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        result = status == -1 ? 1 : static_cast<int>(message.wParam);
    }
    if (menuFont) DeleteObject(menuFont);
    if (menuBrush) DeleteObject(menuBrush);
    if (trayIcon) DestroyIcon(trayIcon);
    if (SUCCEEDED(com)) CoUninitialize();
    if (mutex) CloseHandle(mutex);
    return result;
}
#endif




