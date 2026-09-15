#define LAUNCHER_TEST
#include "../main.cpp"
#include <cstdio>
#include <stdexcept>

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

int main() {
    using namespace launcher;
    try {
        std::vector<Entry> entries;
        for (const auto& sample : {
            std::pair{L"01 Medienbearbeitung.lnk", false},
            std::pair{L"03 Aufnahmenverwaltung.lnk", false},
            std::pair{L"System", true},
            std::pair{L"10 Tools", true},
            std::pair{L"02_Dokumente", true},
            std::pair{L"1-Programme", true},
            std::pair{L"0 Favoriten", true},
            std::pair{L"3 Bilder", true}}) {
            Entry entry;
            entry.filename = sample.first;
            entry.directory = sample.second;
            SetLabel(entry);
            entries.push_back(entry);
        }
        std::sort(entries.begin(), entries.end(), EntryLess);
        const wchar_t* expected[] = {L"Favoriten", L"Programme", L"Dokumente",
            L"Bilder", L"Tools", L"System", L"Medienbearbeitung", L"Aufnahmenverwaltung"};
        for (size_t i = 0; i < entries.size(); ++i)
            Require(entries[i].label == expected[i], "Folder ordering or hidden numeric prefix");

        HDC dc = CreateCompatibleDC(nullptr);
        BITMAPINFO bitmapInfo{};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = 420;
        bitmapInfo.bmiHeader.biHeight = -240;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        void* pixels = nullptr;
        HBITMAP bitmap = CreateDIBSection(dc, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0);
        Require(dc && bitmap, "Create menu render surface");
        HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
        menuBrush = CreateSolidBrush(Background);
        menuFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        MenuItem item;
        item.submenu = true;
        item.label = L"System";
        for (UINT testDpi : {96u, 144u, 192u}) {
            dpi = testDpi;
            for (UINT selection : {0u, static_cast<UINT>(ODS_SELECTED)}) {
                int saved = SaveDC(dc);
                DRAWITEMSTRUCT draw{};
                draw.CtlType = ODT_MENU;
                draw.hDC = dc;
                draw.rcItem = {0, 0, Scale(200), Scale(30)};
                draw.itemData = reinterpret_cast<ULONG_PTR>(&item);
                draw.itemState = selection;
                Draw(&draw);
                int tipX = draw.rcItem.right - Scale(13) + Scale(1);
                int centerY = draw.rcItem.bottom / 2;
                Require(!PtVisible(dc, draw.rcItem.right - Scale(4), centerY),
                    "Native triangle must be clipped");
                Require(PtVisible(dc, 10, draw.rcItem.bottom + 5),
                    "Next menu row must remain drawable");
                RestoreDC(dc, saved);
                Require(GetPixel(dc, tipX, centerY) == Foreground, "Light chevron must be visible");
            }
        }
        SelectObject(dc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(dc);
        DeleteObject(menuBrush);
        std::puts("PASS: folders first, hidden numeric prefixes, one light chevron at 100/150/200% DPI (selected and unselected).");
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
    return 0;
}
