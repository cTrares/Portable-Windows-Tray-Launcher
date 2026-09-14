#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <shellapi.h>
#include <string>

// Harmless executable fixture: prove that the shell actually launched it.
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    wchar_t path[32768]{};
    GetModuleFileNameW(nullptr, path, 32768);
    std::wstring base = path;
    base.resize(base.find_last_of(L'\\') + 1);
    int count = 0;
    wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &count);
    base += count > 1 ? args[1] : L"exe.opened";
    HANDLE file = CreateFileW(base.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    LocalFree(args);
    return file == INVALID_HANDLE_VALUE ? 1 : 0;
}
